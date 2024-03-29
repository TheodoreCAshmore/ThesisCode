package generic

import (
	"context"
	"database/sql"
	"errors"
	"fmt"
	"sync/atomic"
	"time"

	"github.com/k3s-io/kine/pkg/commsdb"
	pb "github.com/k3s-io/kine/pkg/drivers/messagestructures"
	"github.com/k3s-io/kine/pkg/server"
	"github.com/k3s-io/kine/pkg/util"
	"github.com/sirupsen/logrus"
)

// explicit interface check
var _ server.Transaction = (*Tx)(nil)
var counter int64

var notInitialised atomic.Bool

// var blockInitialisation sync.Mutex

// ErrDdsFailed is returned when a DDS operation returns a succeed value of false
// Used by the DDS modification
var ErrDdsFailed = errors.New("DDS: operation failed")

type Tx struct {
	x *sql.Tx
	d *Generic
}

// MEASUREMENT CODE
// Used for latency and throughput measurement
// var nameIdentifierWrite = "KineWrite"
// var nameIdentifierQueryGeneric = "KineReadQueryGeneric"
// var nameIdentifierQueryThree = "KineReadQueryThree"
// var nameIdentifierQueryCompactRevision = "KineReadQueryCompactRevision"
// var nameIdentifierQueryMaxID = "KineReadQueryMaxId"
// var measurementVector []string

// var waitgroupPrint sync.WaitGroup
// var protectPrint sync.Mutex

// func catchSignal() {
// 	sigchan := make(chan os.Signal, 1)
// 	signal.Notify(sigchan, os.Interrupt, syscall.SIGINT, syscall.SIGTERM)
// 	<-sigchan
// 	time.Sleep(1 * time.Second)
// 	os.Exit(0)
// }

// func measurementSetup() {
// 	if !notInitialised.Load() {
// 		notInitialised.Store(true)
// 		go catchSignal()
// 	}
// }

func TestConnection(msg string) {
	s := "Message: " + msg
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	response, err := commsdb.Client.RequestTest(ctx, &pb.KineTest{
		Testid:  counter,
		Content: s,
	})

	if err != nil {
		logrus.Errorf("TestConnection failed due to a RequestTest error.")
		return
	}

	str := fmt.Sprintf("Response returned with id = %d and content of \"%s\"", response.Responseid, response.Content)
	logrus.Debug(str)
	atomic.AddInt64(&counter, 1)
}

func ConnectDdsRequestPublish(id int64, name string, created int64, deleted int64, createRevision int64, prevRevision int64, lease int64, value []byte, oldValue []byte) (bool, int64, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	// Missing publications is too risky, therefore the deadline is increased
	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(4000*time.Millisecond))
	defer cancel()

	// MEASUREMENT CODE
	// measurementStart := rdpmctsc.RdPmcTsc()
	response, err := commsdb.ClientPublisher.RequestPublish(ctx, &pb.KineInsert{
		Id:             id,
		Name:           name,
		Created:        created,
		Deleted:        deleted,
		CreateRevision: createRevision,
		PrevRevision:   prevRevision,
		Lease:          lease,
		Value:          value,
		OldValue:       oldValue,
	})
	// MEASUREMENT CODE
	// measurementEnd := rdpmctsc.RdPmcTsc()
	if err != nil {
		logrus.Errorf("ConnectDdsRequestPublish went wrong. Error is: %s.", err)
		str := fmt.Sprintf("# Published failed when using the following parameters: id = %d, name = %s, created = %d, deleted = %d, create_revision = %d, prev_revision = %d, lease = %d, value = %v, old_value = %v.", id, name, created, deleted, createRevision, prevRevision, lease, value, oldValue)
		logrus.Errorf(str)
		return false, -1, err
	}

	// MEASUREMENT CODE
	// protectPrint.Lock()
	// fmt.Printf("%s,%d,%d,%d\n", nameIdentifierWrite, *response.ReturningId, measurementStart, measurementEnd) // Latency
	// fmt.Printf("%s,%d\n", nameIdentifierPublish, measurementEnd) // Throughput
	// protectPrint.Unlock()
	return true, *response.ReturningId, err
}

func ConnectDdsRequestDispose(idToDispose int64) (bool, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	ctx, cancel := context.WithTimeout(context.Background(), 4*time.Second)
	defer cancel()

	// MEASUREMENT CODE
	// measurementStart := rdpmctsc.RdPmcTsc()

	response, err := commsdb.ClientPublisher.RequestDispose(ctx, &pb.KineDispose{
		Id: idToDispose,
	})

	// MEASUREMENT CODE
	// measurementEnd := rdpmctsc.RdPmcTsc()

	if err != nil {
		return false, err
	}

	// MEASUREMENT CODE
	// protectPrint.Lock()
	// fmt.Printf("%s,%d,%d,%d\n", nameIdentifierWrite, idToDispose, measurementStart, measurementEnd) // Latency
	// fmt.Printf("%s,%d\n", nameIdentifierDispose, measurementEnd) // Throughput
	// protectPrint.Unlock()

	return response.Success, err
}

func ConnectDdsRequestMassDisposeCompaction(param1, param2 int64) (bool, int64, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	response, err := commsdb.Client.RequestMassDisposeCompaction(ctx, &pb.KineMassDisposeCompaction{
		Param1: param1,
		Param2: param2,
	})

	if err != nil {
		return false, -1, err
	}

	return response.Success, response.DeletedRows, err
}

func ConnectDdsRequestQueryGeneric(queryIdentifier int64, param1 string, param2 string, param3 string, param4 string, param5 string, limit int64) ([]*pb.QueryGenericRow, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	// Limit == 0 means that the query has no limit
	if limit == 0 {
		limit = -1
	}

	times := 2
	currentClient := commsdb.Client
	if queryIdentifier == 4 {
		currentClient = commsdb.ClientQuery04
		times = 10
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(times)*time.Second)
	defer cancel()

	// MEASUREMENT CODE
	// measurementStart := rdpmctsc.RdPmcTsc()

	response, err := currentClient.RequestQueryGeneric(ctx, &pb.KineQueryGeneric{
		QueryIdentifier: int32(queryIdentifier),
		Param1:          param1,
		Param2:          param2,
		Param3:          param3,
		Param4:          param4,
		Param5:          param5,
		Limit:           limit,
	})

	// MEASUREMENT CODE
	// measurementEnd := rdpmctsc.RdPmcTsc()

	if err != nil {
		logrus.Errorf("ConnectDdsRequestGeneric went wrong for query %d.", queryIdentifier)
		return nil, err
	}

	// MEASUREMENT CODE
	// protectPrint.Lock()
	// fmt.Printf("%s%d,-,%d,%d\n", nameIdentifierQueryGeneric, queryIdentifier, measurementStart, measurementEnd) // Latency
	// fmt.Printf("%s%d,%d\n", nameIdentifierQueryGeneric, queryIdentifier, measurementEnd) // Throughput
	// protectPrint.Unlock()

	return response.Row, err
}

func ConnectDdsRequestQueryThree(param1 string, param2 string) (int64, int64, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	// MEASUREMENT CODE
	// measurementStart := rdpmctsc.RdPmcTsc()

	response, err := commsdb.Client.RequestQueryThree(ctx, &pb.KineQueryThree{
		Param1: param1,
		Param2: param2,
	})

	// MEASUREMENT CODE
	// measurementEnd := rdpmctsc.RdPmcTsc()

	if err != nil {
		return -1, -1, err
	}

	// MEASUREMENT CODE
	// protectPrint.Lock()
	// fmt.Printf("%s,-,%d,%d\n", nameIdentifierQueryThree, measurementStart, measurementEnd) // Latency
	// fmt.Printf("%s,%d\n", nameIdentifierQueryThree, measurementEnd) // Throughput
	// protectPrint.Unlock()

	return response.Id, response.Count, err
}

func ConnectDdsRequestQueryCompactRevision() (int64, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	// MEASUREMENT CODE
	// measurementStart := rdpmctsc.RdPmcTsc()

	response, err := commsdb.ClientSmallQueries.RequestQueryCompactRevision(ctx, &pb.KineQueryCompactRevision{})

	// MEASUREMENT CODE
	// measurementEnd := rdpmctsc.RdPmcTsc()

	if err != nil {
		return -1, err
	}

	// MEASUREMENT CODE
	// protectPrint.Lock()
	// fmt.Printf("%s,-,%d,%d\n", nameIdentifierQueryCompactRevision, measurementStart, measurementEnd) // Latency
	// fmt.Printf("%s,%d\n", nameIdentifierQueryCompactRevision, measurementEnd) // Throughput
	// protectPrint.Unlock()

	return response.Id, err
}

func ConnectDdsRequestQueryMaximumID() (int64, error) {
	// MEASUREMENT CODE
	// if !notInitialised.Load() {
	// 	measurementSetup()
	// }

	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	// MEASUREMENT CODE
	// measurementStart := rdpmctsc.RdPmcTsc()

	response, err := commsdb.ClientSmallQueries.RequestQueryMaximumId(ctx, &pb.KineQueryMaximumId{})

	if err != nil {
		return -1, err
	}

	// MEASUREMENT CODE
	// measurementEnd := rdpmctsc.RdPmcTsc()

	// MEASUREMENT CODE
	// protectPrint.Lock()
	// fmt.Printf("%s,-,%d,%d\n", nameIdentifierQueryMaxID, measurementStart, measurementEnd) // Latency
	// fmt.Printf("%s,%d\n", nameIdentifierQueryMaxID, measurementEnd) // Throughput
	// protectPrint.Unlock()

	return response.Id, err
}

// Not needed for DDS
func (d *Generic) BeginTx(ctx context.Context, opts *sql.TxOptions) (server.Transaction, error) {
	logrus.Tracef("TX BEGIN")
	return nil, nil
}

// Not needed for DDS
func (t *Tx) Commit() error {
	logrus.Tracef("TX COMMIT")
	return t.x.Commit()
}

// Not needed for DDS
func (t *Tx) MustCommit() {
	if err := t.Commit(); err != nil {
		logrus.Fatalf("Transaction commit failed: %v", err)
	}
}

// Not needed for DDS
func (t *Tx) Rollback() error {
	return nil
}

// Not needed for DDS
func (t *Tx) MustRollback() {
	// if err := t.Rollback(); err != nil {
	// 	if err != sql.ErrTxDone {
	// 		logrus.Fatalf("Transaction rollback failed: %v", err)
	// 	}
	// }
	return
}

/**
 * Same logic as in generic.go
 */
func (t *Tx) GetCompactRevision(ctx context.Context) (int64, error) {
	res, err := ConnectDdsRequestQueryCompactRevision()
	var id = res
	if err != nil {
		// FIXME: This 0 is here due to a lack of a better thing. It should't really be a problem, considering that no row
		// should have a 0 id, but still, better fix it at some point.
		id = 0
		logrus.Errorf("GetCompactRevision in tx.go failed due to a ConnectDdsRequestQueryComnpactRevision error.")
	} else if id < 0 {
		// FIXME: This 0 is here due to a lack of a better thing. It should't really be a problem, considering that no row
		// should have a 0 id, but still, better fix it at some point.
		id = 0
		logrus.Errorf("GetCompactRevision in tx.go failed due to a DDS error.")
		return 0, nil
	}
	return id, err
}

/**
 * Same logic as in generic.go
 */
func (t *Tx) SetCompactRevision(ctx context.Context, revision int64) error {
	logrus.Tracef("TX SETCOMPACTREVISION %v", revision)
	_, err := t.execute(ctx, t.d.UpdateCompactSQL, revision)

	id, err := ConnectDdsRequestQueryCompactRevision()
	succeeded, _, err := ConnectDdsRequestPublish(id, "compact_rev_key", 1, 0, 0, revision, 0, []byte("\\x"), []byte(""))
	if err != nil {
		logrus.Errorf("SetCompactRevision in tx failed due to a ConnectDdsRequestPublish error.")
	} else if !succeeded {
		err = ErrDdsFailed
		logrus.Errorf("SetCompactRevision in tx failed due to a DDS error.")
	}

	return err
}

// TODO: The compaction mechanism is not implemented
func (t *Tx) Compact(ctx context.Context, revision int64) (int64, error) {
	// TODO: uncomment that if the massdeletion in DDS is implemented
	// succeeded, rowsDeleted, err := ConnectDdsRequestMassDisposeCompaction(revision, revision) // TODO: Sending the same value twice is useless, gRPC message needs to be changed

	// if err != nil {
	// 	logrus.Errorf("Compact in tx failed due to a ConnectDdsRequestMassDisposeCompaction error.")
	// 	return 0, err
	// } else if !succeeded {
	// 	err = ErrDdsFailed
	// 	logrus.Errorf("Compact in tx failed due to a DDS error.")
	// 	return 0, err
	// }
	// return rowsDeleted, err
	logrus.Errorf("USED Compact IN tx.go!")
	logrus.Tracef("TX COMPACT %v", revision)
	res, err := t.execute(ctx, t.d.CompactSQL, revision, revision)
	if err != nil {
		return 0, err
	}
	return res.RowsAffected()
}

func (t *Tx) GetRevision(ctx context.Context, revision int64) (*sql.Rows, error) {
	logrus.Errorf("USED GetRevision IN tx.go!")
	return t.query(ctx, t.d.GetRevisionSQL, revision)
}

/**
 * Same logic as in generic.go
 */
func (t *Tx) DeleteRevision(ctx context.Context, revision int64) error {
	succeeded, err := ConnectDdsRequestDispose(revision)
	if err != nil || !succeeded {
		logrus.Errorf("DeleteRevision in tx failed due to a ConnectDdsRequestDispose error.")
	} else if !succeeded {
		err = ErrDdsFailed
		logrus.Errorf("DeleteRevision in tx failed due to a DDS error.")
	}

	return err
}

/**
 * Same logic as in generic.go
 */
func (t *Tx) CurrentRevision(ctx context.Context) (int64, error) {
	maxID, err := ConnectDdsRequestQueryMaximumID()

	if err != nil {
		logrus.Errorf("CurrentRevision in tx failed due to a ConnectDdsRequestQueryMaximumID error.")
	}

	return maxID, err
}

func (t *Tx) query(ctx context.Context, sql string, args ...interface{}) (result *sql.Rows, err error) {
	logrus.Tracef("TX QUERY %v : %s", args, util.Stripped(sql))
	return t.x.QueryContext(ctx, sql, args...)
}

func (t *Tx) queryRow(ctx context.Context, sql string, args ...interface{}) (result *sql.Row) {
	logrus.Tracef("TX QUERY ROW %v : %s", args, util.Stripped(sql))
	return t.x.QueryRowContext(ctx, sql, args...)
}

func (t *Tx) execute(ctx context.Context, sql string, args ...interface{}) (result sql.Result, err error) {
	logrus.Tracef("TX EXEC %v : %s", args, util.Stripped(sql))
	return t.x.ExecContext(ctx, sql, args...)
}
