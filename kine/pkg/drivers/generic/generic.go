package generic

import (
	"context"
	"database/sql"
	"errors"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/Rican7/retry/backoff"
	"github.com/Rican7/retry/strategy"
	"github.com/k3s-io/kine/pkg/drivers/messagestructures"
	"github.com/k3s-io/kine/pkg/server"
	"github.com/k3s-io/kine/pkg/util"
	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/collectors"
	"github.com/sirupsen/logrus"
)

const (
	defaultMaxIdleConns = 2 // copied from database/sql
)

// explicit interface check
var _ server.Dialect = (*Generic)(nil)

var (
	columns = "kv.id AS theid, kv.name, kv.created, kv.deleted, kv.create_revision, kv.prev_revision, kv.lease, kv.value, kv.old_value"
	revSQL  = `
		SELECT MAX(rkv.id) AS id
		FROM kine AS rkv`

	compactRevSQL = `
		SELECT MAX(crkv.prev_revision) AS prev_revision
		FROM kine AS crkv
		WHERE crkv.name = 'compact_rev_key'`

	idOfKey = `
		AND
		mkv.id <= ? AND
		mkv.id > (
			SELECT MAX(ikv.id) AS id
			FROM kine AS ikv
			WHERE
				ikv.name = ? AND
				ikv.id <= ?)`

	listSQL = fmt.Sprintf(`
		SELECT *
		FROM (
			SELECT (%s), (%s), %s
			FROM kine AS kv
			JOIN (
				SELECT MAX(mkv.id) AS id
				FROM kine AS mkv
				WHERE
					mkv.name LIKE ?
					%%s
				GROUP BY mkv.name) AS maxkv
				ON maxkv.id = kv.id
			WHERE
				kv.deleted = 0 OR
				?
		) AS lkv
		ORDER BY lkv.theid ASC
		`, revSQL, compactRevSQL, columns)
)

type ErrRetry func(error) bool
type TranslateErr func(error) error
type ErrCode func(error) string

type ConnectionPoolConfig struct {
	MaxIdle     int           // zero means defaultMaxIdleConns; negative means 0
	MaxOpen     int           // <= 0 means unlimited
	MaxLifetime time.Duration // maximum amount of time a connection may be reused
}

type Generic struct {
	sync.Mutex

	LockWrites            bool
	LastInsertID          bool
	DB                    *sql.DB
	GetCurrentSQL         string
	GetRevisionSQL        string
	RevisionSQL           string
	ListRevisionStartSQL  string
	GetRevisionAfterSQL   string
	CountSQL              string
	AfterSQL              string
	DeleteSQL             string
	CompactSQL            string
	UpdateCompactSQL      string
	PostCompactSQL        string
	InsertSQL             string
	FillSQL               string
	InsertLastInsertIDSQL string
	GetSizeSQL            string
	Retry                 ErrRetry
	TranslateErr          TranslateErr
	ErrCode               ErrCode
}

/**
 * This method is used to format the text of query.
 * Unused for DDS
 */
func q(sql, param string, numbered bool) string {
	if param == "?" && !numbered {
		return sql
	}

	regex := regexp.MustCompile(`\?`)
	n := 0
	return regex.ReplaceAllStringFunc(sql, func(string) string {
		if numbered {
			n++
			return param + strconv.Itoa(n)
		}
		return param
	})
}

/**
 * Used for migration purposes
 * Irrelevant for DDS purposes, removed
 */
func (d *Generic) Migrate(ctx context.Context) {
	return
}

/**
 * Deleting this may break internal Kine things, left as is to prevent obscure errors
 */
func configureConnectionPooling(connPoolConfig ConnectionPoolConfig, db *sql.DB, driverName string) {
	// behavior copied from database/sql - zero means defaultMaxIdleConns; negative means 0
	if connPoolConfig.MaxIdle < 0 {
		connPoolConfig.MaxIdle = 0
	} else if connPoolConfig.MaxIdle == 0 {
		connPoolConfig.MaxIdle = defaultMaxIdleConns
	}

	logrus.Infof("Configuring %s database connection pooling: maxIdleConns=%d, maxOpenConns=%d, connMaxLifetime=%s", driverName, connPoolConfig.MaxIdle, connPoolConfig.MaxOpen, connPoolConfig.MaxLifetime)
	db.SetMaxIdleConns(connPoolConfig.MaxIdle)
	db.SetMaxOpenConns(connPoolConfig.MaxOpen)
	db.SetConnMaxLifetime(connPoolConfig.MaxLifetime)
}

/**
 * Connection and driver configuration.
 * May break things, left untouched
 */
func openAndTest(driverName, dataSourceName string) (*sql.DB, error) {
	db, err := sql.Open(driverName, dataSourceName)
	if err != nil {
		return nil, err
	}

	for i := 0; i < 3; i++ {
		if err := db.Ping(); err != nil {
			db.Close()
			return nil, err
		}
	}

	return db, nil
}

/**
 * Fills the Generic struct with the sql code and other necessary data for the database.
 * May break things, left untouched
 */
func Open(ctx context.Context, driverName, dataSourceName string, connPoolConfig ConnectionPoolConfig, paramCharacter string, numbered bool, metricsRegisterer prometheus.Registerer) (*Generic, error) {
	var (
		db  *sql.DB
		err error
	)

	// Sets up the database connections
	for i := 0; i < 300; i++ {
		db, err = openAndTest(driverName, dataSourceName)
		if err == nil {
			break
		}

		// logrus.Errorf("failed to ping connection: %v", err)
		select {
		case <-ctx.Done():
			return nil, nil
		case <-time.After(time.Second):
			return nil, nil
		}
	}

	configureConnectionPooling(connPoolConfig, db, driverName)

	// Sets up the metrics server
	if metricsRegisterer != nil {
		metricsRegisterer.MustRegister(collectors.NewDBStatsCollector(db, "kine"))
	}

	// Here a Generic struct is created
	// All the SQL queries are saved here
	return &Generic{
		DB: db,

		GetRevisionSQL: q(fmt.Sprintf(`
			SELECT
			0, 0, %s
			FROM kine AS kv
			WHERE kv.id = ?`, columns), paramCharacter, numbered),

		GetCurrentSQL:        q(fmt.Sprintf(listSQL, ""), paramCharacter, numbered),
		ListRevisionStartSQL: q(fmt.Sprintf(listSQL, "AND mkv.id <= ?"), paramCharacter, numbered),
		GetRevisionAfterSQL:  q(fmt.Sprintf(listSQL, idOfKey), paramCharacter, numbered),

		CountSQL: q(fmt.Sprintf(`
			SELECT (%s), COUNT(c.theid)
			FROM (
				%s
			) c`, revSQL, fmt.Sprintf(listSQL, "")), paramCharacter, numbered),

		AfterSQL: q(fmt.Sprintf(`
			SELECT (%s), (%s), %s
			FROM kine AS kv
			WHERE
				kv.name LIKE ? AND
				kv.id > ?
			ORDER BY kv.id ASC`, revSQL, compactRevSQL, columns), paramCharacter, numbered),

		DeleteSQL: q(`
			DELETE FROM kine AS kv
			WHERE kv.id = ?`, paramCharacter, numbered),

		UpdateCompactSQL: q(`
			UPDATE kine
			SET prev_revision = ?
			WHERE name = 'compact_rev_key'`, paramCharacter, numbered),

		InsertLastInsertIDSQL: q(`INSERT INTO kine(name, created, deleted, create_revision, prev_revision, lease, value, old_value)
			values(?, ?, ?, ?, ?, ?, ?, ?)`, paramCharacter, numbered),

		InsertSQL: q(`INSERT INTO kine(name, created, deleted, create_revision, prev_revision, lease, value, old_value)
			values(?, ?, ?, ?, ?, ?, ?, ?) RETURNING id`, paramCharacter, numbered),

		FillSQL: q(`INSERT INTO kine(id, name, created, deleted, create_revision, prev_revision, lease, value, old_value)
			values(?, ?, ?, ?, ?, ?, ?, ?, ?)`, paramCharacter, numbered),
	}, nil
}

/**
 * Method called by other methods when they want to run a query that may return more than one row.
 * It returns Rows
 * Unused in DDS
 */
func (d *Generic) query(ctx context.Context, sql string, args ...interface{}) (result *sql.Rows, err error) {
	logrus.Tracef("QUERY %v : %s", args, util.Stripped(sql))
	return d.DB.QueryContext(ctx, sql, args...)
}

/**
 * Method called by other methods when they want to run a query that only returns one row.
 * Unused in DDS
 */
func (d *Generic) queryRow(ctx context.Context, sql string, args ...interface{}) (result *sql.Row) {
	logrus.Tracef("QUERY ROW %v : %s", args, util.Stripped(sql))
	return d.DB.QueryRowContext(ctx, sql, args...)
}

/**
 * Method called by other methods to issue queries of type INSERT, UPDATE or DELETE (writing queries).
 * Unused in DDS
 */
func (d *Generic) execute(ctx context.Context, sql string, args ...interface{}) (result sql.Result, err error) {
	if d.LockWrites {
		d.Lock()
		defer d.Unlock()
	}

	wait := strategy.Backoff(backoff.Linear(100 + time.Millisecond))

	for i := uint(0); i < 20; i++ {
		logrus.Tracef("EXEC (try: %d) %v : %s", i, args, util.Stripped(sql))
		result, err = d.DB.ExecContext(ctx, sql, args...)
		if err != nil && d.Retry != nil && d.Retry(err) {
			wait(i)
			continue
		}
		return result, err
	}
	return
}

/**
 * Gets the current id of the compact revision (row named "compact_rev_key")
 */
func (d *Generic) GetCompactRevision(ctx context.Context) (int64, error) {
	// Request the data from DDS
	res, err := ConnectDdsRequestQueryCompactRevision()
	var id = res
	if err != nil {
		logrus.Errorf("GetCompactRevision failed due to an error in ConnectDdsRequestQueryCompactRevision.")
	} else if id < 0 {
		return 0, nil
	}

	return id, err
}

/**
 * Changes the prev_revision field of the row named "compact_rev_key". Used during compaction.
 * Needs to republish a message with the same id of the compact_rev_key row that already exists, changing the prev_revision field
 */
func (d *Generic) SetCompactRevision(ctx context.Context, revision int64) error {
	id, err := ConnectDdsRequestQueryCompactRevision()
	succeeded, _, err := ConnectDdsRequestPublish(id, "compact_rev_key", 1, 0, 0, revision, 0, []byte("\\x"), []byte(""))
	if err != nil {
		logrus.Errorf("Publish for SetCompactRevision failed due to an error in ConnectDdsRequestPublish.")
	} else if !succeeded {
		err = ErrDdsFailed
		logrus.Errorf("Publish for SetCompactRevision failed due to a DDS error.")
	}
	return err
}

/**
 * Deletes rows when compaction occurs. Possibly unused.
 * Unused in DDS
 */
func (d *Generic) Compact(ctx context.Context, revision int64) (int64, error) {
	return 0, nil
}

/**
 * Used to execute any database cleanup after a compaction (vacuuming, WAL truncate, etc.). Only used during compaction.
 * Irrelevant for DDS, returns nil.
 */
func (d *Generic) PostCompact(ctx context.Context) error {
	logrus.Trace("POSTCOMPACT")
	return nil
}

/**
 * Gets a row (or rows) with a specified id.
 * No occurrences of it have been seen in any run. Believed unused
 */
func (d *Generic) GetRevision(ctx context.Context, revision int64) (*sql.Rows, error) {
	logrus.Errorf("USED GetRevision IN generic.go!")
	return d.query(ctx, d.GetRevisionSQL, revision)
}

/**
 * Deletes a row by its id. Used during compaction.
 * Needs to issue a deletion to the DDS database.
 */
func (d *Generic) DeleteRevision(ctx context.Context, revision int64) error {
	// Connection with the DDS datbase
	succeeded, err := ConnectDdsRequestDispose(revision)
	if err != nil || !succeeded {
		logrus.Errorf("Dispose for DeleteRevision failed due to a ConnectDdsRequestDispose error.")
	} else if !succeeded {
		err = ErrDdsFailed
		logrus.Errorf("Dispose for DeleteRevision failed due to a DDS error.")
	}

	return err
}

/**
 * Issues a Query04 (GetCurrentSQL) with the specified limit.
 * Needs to get the data from the DDS database.
 */
func (d *Generic) ListCurrent(ctx context.Context, prefix string, limit int64, includeDeleted bool) ([]*messagestructures.QueryGenericRow, error) {
	// Limit == 0 means that the query has no limit
	return ConnectDdsRequestQueryGeneric(4, prefix, parseIcludeDelete(includeDeleted), "", "", "", limit)
}

/**
 * Issues a Query11 (ListRevisionStartSQL) with the specified limit if startKey is "".
 * Otherwise, a Query09 (GetRevisionAfterSQL) is issued, also with a limit.
 * Needs to get the data from the DDS database.
 */
func (d *Generic) List(ctx context.Context, prefix, startKey string, limit, revision int64, includeDeleted bool) ([]*messagestructures.QueryGenericRow, error) {
	if startKey == "" {
		return ConnectDdsRequestQueryGeneric(11, prefix, strconv.FormatInt(revision, 10), parseIcludeDelete(includeDeleted), "", "", limit)
	}
	return ConnectDdsRequestQueryGeneric(9, prefix, strconv.FormatInt(revision, 10), startKey, strconv.FormatInt(revision, 10), parseIcludeDelete(includeDeleted), limit)
}

/**
 * Issues a Query03 (CountSQL) where prefix is parameter $1 and parameter $2 is always false ('f' when sent over).
 * First value returned is the count, second the Maximum id
 */
func (d *Generic) Count(ctx context.Context, prefix string) (int64, int64, error) {
	maxID, count, err := ConnectDdsRequestQueryThree(prefix, "'f'")

	if err != nil {
		logrus.Errorf("Count failed due to a ConnectDdsRequestQueryThree error.")
	} else if maxID < 0 {
		err = ErrDdsFailed
		logrus.Errorf("Count failed due to DDS error: it returned %d.", count)
	}

	return count, maxID, err
}

/**
 * Issues a query to get the maximum id in the database.
 * Needs to get the data from the DDS databse.
 */
func (d *Generic) CurrentRevision(ctx context.Context) (int64, error) {
	maxID, err := ConnectDdsRequestQueryMaximumID()

	if err != nil {
		logrus.Errorf("CurrentRevision failed due to a ConnectDdsRequestQueryMaximumID error.")
	}

	return maxID, err
}

/**
 * Launches a Query01 (AfterSQL).
 * Needs to get the data from the DDS database.
 */
func (d *Generic) After(ctx context.Context, prefix string, rev, limit int64) ([]*messagestructures.QueryGenericRow, error) {
	res, err := ConnectDdsRequestQueryGeneric(1, prefix, strconv.FormatInt(rev, 10), "", "", "", limit)
	return res, err
}

/**
 * Inserts a filler row with the specified id (name begins with "gap-" and all fields are 0 but the deleted one, which is 1)
 * Needs to send the data to be logged into the DDS databse
 */
func (d *Generic) Fill(ctx context.Context, revision int64) error {
	// Connection with the DDS datbase
	name := fmt.Sprintf("gap-%d", revision)
	succeeded, _, err := ConnectDdsRequestPublish(revision, name, 0, 1, 0, 0, 0, []byte(""), []byte(""))
	if err != nil {
		logrus.Errorf("Fill failed due to a ConnectDdsRequestPublish error.")
	} else if !succeeded {
		err = ErrDdsFailed
		logrus.Errorf("Fill failed due to a DDS error.")
	}

	return err
}

/**
 * Checks if a string begins with the prefix gap
 */
func (d *Generic) IsFill(key string) bool {
	return strings.HasPrefix(key, "gap-")
}

/**
 * Makes an insert into the databse
 * Needs to send the data to be logged into the DDS databse
 */
func (d *Generic) Insert(ctx context.Context, key string, create, delete bool, createRevision, previousRevision int64, ttl int64, value, prevValue []byte) (id int64, e error) {
	cVal := 0
	dVal := 0
	if create {
		cVal = 1
	}
	if delete {
		dVal = 1
	}
	// Connection with the DDS databse
	// d.InsertLastInsertIDSQL doesn't specify the id, instead it lets the SERIAL id do the increment
	// Therefore, the id sent is -1 in order to simulate the serial increment
	isOk, id, err := ConnectDdsRequestPublish(-1, key, int64(cVal), int64(dVal), createRevision, previousRevision, ttl, value, prevValue)
	if !isOk {
		id = 0
		logrus.Errorf("Error in ConnectDdsRequestPublish within Insert")
	}
	if id < 0 {
		id = 0
		err = ErrDdsFailed
		logrus.Errorf("Error in DDS within Insert")
	}

	return id, err
}

/**
 * Measures the size of the database for status purposes.
 * Modified to indicate that the database does not support this operation.
 */
func (d *Generic) GetSize(ctx context.Context) (int64, error) {
	return 0, errors.New("driver does not support size reporting")
}

// Used to format the sent data into something DDS understands
func parseIcludeDelete(includeDelete bool) string {
	if includeDelete {
		return "t"
	}
	return "f"
}
