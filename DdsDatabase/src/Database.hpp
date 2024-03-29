/**
 * @file Database.cpp
 * @author Saül Abad Copoví
 * @brief DataBase header
 */
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <csignal>

#include "dds/dds.hpp"
#include "DatabaseData.hpp"

#include "QueryStructure.h"

using namespace org::eclipse::cyclonedds;

class Database
{
private:
    // +-----------+
    // | VARIABLES |
    // +-----------+
    //! Declaration of the basic entities of the DDS database
    std::shared_ptr<dds::domain::DomainParticipant> participant_;
    std::shared_ptr<dds::topic::Topic<DatabaseData::Msg>> topic_;
    std::shared_ptr<dds::pub::Publisher> publisher_;
    std::shared_ptr<dds::sub::Subscriber> subscriber_;
    std::shared_ptr<dds::pub::DataWriter<DatabaseData::Msg>> writer_;
    std::shared_ptr<dds::sub::DataReader<DatabaseData::Msg>> reader_;
    //! Reader handle
    std::shared_ptr<dds_entity_t> reader_handle_;
    std::shared_ptr<dds_entity_t> reader_maxid_handle_;
    //! Declaration of the entities of the MaxId topic
    std::shared_ptr<dds::domain::DomainParticipant> participant_maxid_;
    std::shared_ptr<dds::topic::Topic<IdentifierTracker::MaxIdMsg>> topic_maxid_;
    std::shared_ptr<dds::pub::Publisher> publisher_maxid_;
    std::shared_ptr<dds::sub::Subscriber> subscriber_maxid_;
    std::shared_ptr<dds::pub::DataWriter<IdentifierTracker::MaxIdMsg>> writer_maxid_;
    std::shared_ptr<dds::sub::DataReader<IdentifierTracker::MaxIdMsg>> reader_maxid_;
    //! Other variables
    bool done_;            // Used to kill the database gracefully
    int maximum_messages_; // Maximum number of messages expected (due to using C arrays for QueryConditions)

    // +--------------------+
    // | LISTENER FUNCTIONS |
    // +--------------------+
    /**
     * @brief Custom DataReader listener function, reacts to new messages received. Debugging purposes.
     *
     * @param reader Reference to the reader which will execute the callback function
     * @return true
     * @return false
     */
    static bool listener_ondataavailable(dds::sub::DataReader<DatabaseData::Msg> &reader);

    /**
     * @brief Custom DataReader listener function, reacts when a subscription matches
     *
     * @param reader
     * @return true
     * @return false
     */
    static bool listener_subscriptionmatched(dds::sub::DataReader<DatabaseData::Msg> &reader);

    /**
     * @brief Custom DataWriter listener function, reacts when a publication matches
     *
     * @param writer
     * @return true
     * @return false
     */
    static bool listener_publicationmatched(dds::pub::DataWriter<DatabaseData::Msg> &writer);

    static bool listener_ondataavailablemaxid(dds::sub::DataReader<IdentifierTracker::MaxIdMsg> &reader);
    static bool listener_subscriptionmatchedmaxid(dds::sub::DataReader<IdentifierTracker::MaxIdMsg> &reader);
    static bool listener_publicationmatchedmaxid(dds::pub::DataWriter<IdentifierTracker::MaxIdMsg> &writed);

    // +-------------------+
    // | WAITSET FUNCTIONS |
    // +-------------------+
    /**
     * @brief Function that creates a waitset to match DataWriters and DataReaders
     *
     * @param wr Reference to the writer to match
     * @param rd Reference to the reader to match
     * @return true If the match is successfull
     * @return false Otherwise
     */
    static bool match_readers_and_writers(dds::pub::DataWriter<DatabaseData::Msg> &wr, dds::sub::DataReader<DatabaseData::Msg> &rd);

    /**
     * @brief Function that creates a waitset to match DataWriters and DataReaders for the MaxId topic
     *
     * @param wr Reference to the writer to match
     * @param rd Reference to the reader to match
     * @return true If the match is successfull
     * @return false Otherwise
     */
    bool match_readers_and_writers_maxid(dds::pub::DataWriter<IdentifierTracker::MaxIdMsg> &wr, dds::sub::DataReader<IdentifierTracker::MaxIdMsg> &rd);

    // +-----------+
    // | UTILITIES |
    // +-----------+

    static void utils_exception_handler(std::exception_ptr exception_pointer);

    /**
     * @brief Removes the percentage of a string. Used for queries
     *
     * @param s string to remove the percentage from
     * @return std::string
     */
    std::string utils_remove_percentage(std::string s);

    /**
     * @brief Checks if a string has a percentage in it
     *
     * @param s string to check whether it contains a percentage
     * @return true
     * @return false
     */
    bool utils_has_percentage(std::string s);

    // +----------------+
    // | CORE FUNCTIONS |
    // +----------------+
    /**
     * @brief Publishes a message
     *
     * @param id Id field of the message
     * @param name Name field of the message
     * @param created Created field of the message
     * @param deleted Deleted field of the message
     * @param create_revision Create_revision field
     * @param prev_revision Prev_revision field of the message
     * @param lease Lease field of the message
     * @param value Value field of the message
     * @param old_value Old_value field of the message
     * @return std::pair<unsigned long, bool>, first is the id, second is true if the publication of the message succeeds
     */
    std::pair<unsigned long, bool> core_publish(unsigned long id, std::string name,
                                                unsigned long created, unsigned long deleted,
                                                unsigned long create_revision, unsigned long prev_revision,
                                                unsigned long lease,
                                                std::vector<unsigned char> value, std::vector<unsigned char> old_value);

    /**
     * @brief Makes the DataWriter dispose and unregister a message
     *
     * @param data Message of type DatabaseData::Msg with the data of the message to be disposed
     * @return true if the disposition of the message succeeds
     * @return false if the disposition of the message fails
     */
    bool core_dispose(DatabaseData::Msg data);

    /**
     * @brief Publishes a message and gets the corresponding new id
     *
     * @param name Name field of the message
     * @param created Created field of the message
     * @param deleted Deleted field of the message
     * @param create_revision Create_revision field
     * @param prev_revision Prev_revision field of the message
     * @param lease Lease field of the message
     * @param value Value field of the message
     * @param old_value Old_value field of the message
     * @return std::pair<unsigned long, bool>, first is the id, second is true if the publication of the message succeeds
     */
    std::pair<unsigned long, bool> core_publish_returningid(std::string name,
                                                            unsigned long created, unsigned long deleted,
                                                            unsigned long create_revision, unsigned long prev_revision,
                                                            unsigned long lease,
                                                            std::vector<unsigned char> value, std::vector<unsigned char> old_value);

    // +-----------------+
    // | QUERY FUNCTIONS |
    // +-----------------+

    /**
     * @brief Deletes all messages according to CompactSQL used by kine
     *
     * @param param1 $1 used in CompactSQL
     * @param param2 $2 used in CompactSQL
     * @return QueryStructure::structure_massDisposeCompaction, bool that indicates success and int that indicates the number of messages deleted
     */
    QueryStructure::structure_massDisposeCompaction query_mass_dispose_compaction(int param1, int param2);

    /**
     * @brief Finds the message with name 'compact_rev_key'
     *
     * @return DatabaseData::Msg copy of the message named 'compact_rev_key'
     */
    DatabaseData::Msg query_compaction();

    /**
     * @brief Finds a message given it's unique id
     *
     * @return DatabaseData::Msg copy of the message with the given id
     */
    DatabaseData::Msg query_find_message_by_id(unsigned long id);

    /**
     * @brief Returns the highest id
     *
     * @return DatabaseData::Msg, message with highest id or empty message
     */
    std::pair<unsigned long, bool> query_maxid();

    /**
     * @brief Finds the max id and returns maxid + 1
     *
     * @return DatabaseData::Msg, dummy message with the highest id + 1 or empty message
     */
    std::pair<unsigned long, bool> query_maxid_and_replace();

    /**
     * @brief Finds the maximum id and the prev_revision of the message named 'compact_rev_key'
     *
     * @param max_id pointer to an int to store the maximum id
     * @param compaction_prev_revision pointer to an int to store the prev_revision of the compaction row
     */
    void query_find_common_values(unsigned long *max_id, unsigned long *compaction_prev_revision);

    /**
     * @brief Performs the same functionality as an SQL COUNT(*)
     *
     * @return int, result of the count
     */
    int query_count_all();

    /**
     * @brief Used by queries 4, 9 and 11 to take care of their GROUP BY name
     *
     * @param iterations int specifying the number of messages that need to be iterated over, for array traversing purposes
     * @param msgs_infos pointer to the beginning of the array containing the messages' info
     * @param msgs_data pointer to the beginning of the array containing the messages' data
     * @param max_id used to populate the resulting structure_generic
     * @param compaction_prev_revision used to populate the resulting structure_generic
     * @param t_f parameter acting as a boolean that indicates whether messages with deleted==1 are considered (when 'f') or not (when 't')
     * @return std::vector<QueryStructure::structure_generic>, contains the messages agter the GROUP BY name and ordered
     */
    std::vector<QueryStructure::structure_generic> query_common_subquery(
        int iterations, dds_sample_info_t *msgs_infos, DatabaseData::Msg *msgs_data, int max_id, int compaction_prev_revision, std::string t_f);

    /**
     * @brief Performs queries of type Query01 (Query01 & Query02)
     * Returns the maximum id, the prev_revision of the compact_rev_key and the row with the name as $1 and with id > $2
     * @param limit Same as SQL LIMIT. limit < 0 indicates no limit
     * @param result Pointer to a LoanableSequence where the data of the samples will be stored
     * @param infos Pointer to a SampleInfoSeq where the info of the samples will be stored
     * @param parameter_vector String vector with two parameters: $1 is a string, $2 an int
     * @return std::vector<QueryStructure::structure_generic>, vector with the result of the query
     */
    std::vector<QueryStructure::structure_generic> query_01(int limit, std::vector<std::string> parameter_vector);
    /**
     * @brief Performs queries of type Query03
     * @param parameter_vector String vector with two parameters: $1 is a string, $2 a string specifying a bool
     * @return QueryStructure::structure_query03, result of the query (it's always only a row)
     */
    QueryStructure::structure_query03 query_03(std::vector<std::string> parameter_vector);
    /**
     * @brief Performs queries of type Query04
     *
     * @param limit Same as SQL LIMIT. limit < 0 indicates no limit
     * @param parameter_vector String vector with two parameters: $1 is a string, $2 a string specifying a bool
     * @return std::vector<QueryStructure::structure_generic>, vector with the result of the query
     */
    std::vector<QueryStructure::structure_generic> query_04(int limit, std::vector<std::string> parameter_vector);
    /**
     * @brief Performs queries of type Query09
     *
     * @param limit Same as SQL LIMIT. limit < 0 indicates no limit
     * @param parameter_vector String vector with two 5 parameters: $1 is a string, $2 an int, $3 a string, $4 an int, $5 a string specifying a bool
     * @return std::vector<QueryStructure::structure_generic>, vector with the result of the query
     */
    std::vector<QueryStructure::structure_generic> query_09(int limit, std::vector<std::string> parameter_vector);
    /**
     * @brief Performs queries of type Query11
     *
     * @param limit Same as SQL LIMIT. limit < 0 indicates no limit
     * @param parameter_vector String vector with two 3 parameters: $1 is a string, $2 an int, $3 a string specifying a bool
     * @return std::vector<QueryStructure::structure_generic>, vector with the result of the query
     */
    std::vector<QueryStructure::structure_generic> query_11(int limit, std::vector<std::string> parameter_vector);

    // +------------------------+
    // | QUERYCONDITION FILTERS |
    // +------------------------+
    /**
     * @brief Testing filter used in TESTING_query. TODO: delete at some point when all the queries are implemented
     *
     * @param message Message to test
     * @return true
     * @return false
     */
    static bool TESTING_filter_test(const DatabaseData::Msg *message);

    /**
     * @brief Filter used to find messages to be deleted when a compaction is performed:
     *          name != 'compact_rev_key' and prev_rev != 0 and id <= $1
     *          UNION
     *          deleted != 0 and id <= $2
     *
     * @param message
     * @return true
     * @return false
     */
    static bool filter_mass_dispose_compaction_subquery01(const DatabaseData::Msg *message);

    static bool filter_mass_dispose_compaction_subquery02(const DatabaseData::Msg *message);

    /**
     * @brief Filter used to find messages whose name is 'compact_rev_key'
     *
     * @param message
     * @return true
     * @return false
     */
    static bool filter_query_compaction(const DatabaseData::Msg *message);

    /**
     * @brief Filter used to find a message given its id
     *
     * @param message Message to test
     * @return true if the message has the given id
     * @return false otherwise
     */
    static bool filter_query_find_message_by_id(const DatabaseData::Msg *message);

    /**
     * @brief Filter for Query01: name LIKE $1 AND id > $2
     *
     * @param message Message to test
     * @return true if the message matches the QueryCondition
     * @return false if the message doesn't match the QueryCondition
     */
    static bool filter_query01(const DatabaseData::Msg *message);

    /**
     * @brief Filter for Query01: name LIKE $1 AND id > $2. Used when the string has a % at beginning or end
     *
     * @param message Message to test
     * @return true if the message matches the QueryCondition
     * @return false if the message doesn't match the QueryCondition
     */
    static bool filter_query01_percentage(const DatabaseData::Msg *message);

    /**
     * @brief Filter for Query04, subquery01: name LIKE $1
     *
     * @param message
     * @return true
     * @return false
     */
    static bool filter_query04_subquery01(const DatabaseData::Msg *message);

    /**
     * @brief Filter for Query04, subquery01: name LIKE $1
     *
     * @param message
     * @return true
     * @return false
     */
    static bool filter_query04_subquery01_percentage(const DatabaseData::Msg *message);

    /**
     * @brief Filter for Query04, subquery02: deleted = 0 OR $2
     *
     * @param message
     * @return true
     * @return false
     */
    // static bool filter_query04_subquery02(const DatabaseData::Msg *message);

    static bool filter_query09_subquery01(const DatabaseData::Msg *message);

    static bool filter_query09_subquery02(const DatabaseData::Msg *message);
    static bool filter_query09_subquery02_percentage(const DatabaseData::Msg *message);

    static bool filter_query11_subquery01(const DatabaseData::Msg *message);
    static bool filter_query11_subquery01_percentage(const DatabaseData::Msg *message);
    // +-------------------+
    // | TESTING FUNCTIONS |
    // +-------------------+
    /**
     * @brief Prints a vector of type uint8_t (useful to print the fields value and old_value)
     *
     * @param v Vector of type uint8_t
     */
    void TESTING_print_vector(std::vector<uint8_t> v);

    /**
     * @brief Prints a message of type DatabaseData::Msg
     *
     * @param msg Message to print
     */
    void TESTING_print_message(DatabaseData::Msg msg);

    // void TESTING_print_message_sequence(FooSeq *m, SampleInfoSeq *i);

    /**
     * @brief Prints a QueryStructure::structure_generic
     *
     * @param to_print Structure_generic to print
     */
    void TESTING_print_structure_generic(QueryStructure::structure_generic to_print);

    /**
     * @brief Prints a QueryStructure::structure_query03
     *
     * @param to_print Structure_query03 to print
     */
    void TESTING_print_structure_query03(QueryStructure::structure_query03 to_print);

    /**
     * @brief Asks for manual input for the fields and publishes a message with that content (the id can be automatically assigned)
     *
     */
    void TESTING_publish();

    /**
     * @brief Unregisters and disposes a message
     *
     */
    void TESTING_dispose();

    /**
     * @brief Prints the total counted messages with alive instance state
     *
     */
    void TESTING_count();

    /**
     * @brief Prints all history within the readers for the Main topic
     *
     */
    void TESTING_print_all_history();

    /**
     * @brief Prints all history within the readers for the MaxId topic
     *
     */
    void TESTING_print_all_history_maxid();

    /**
     * @brief TODO:
     *
     */
    void TESTING_query();

    /**
     * @brief Prints the maximum id among all messages
     *
     */
    void TESTING_print_max();

    /**
     * @brief Prints all messages with instance state "not alive disposed"
     *
     */
    void TESTING_print_disposed();

    // bool TESTING_change_filter(std::string expression, std::vector<std::string> parameters);

    /**
     * @brief Asks for user input to access one of the testing functions
     *
     */
    void TESTING_get_options();

    /**
     * @brief
     *
     * @param count
     */
    void TESTING_populate(int count);

    void TESTING_update_message();

public:
    /**
     * @brief Construct a new Database object
     *
     */
    Database(int max_messages);

    /**
     * @brief Destroy the Database object
     *
     */
    virtual ~Database();

    /**
     * @brief Initialises the main service and the DB DDS entities
     *
     * @return true
     * @return false
     */
    bool init();

    /**
     * @brief
     *
     * @return true
     * @return false
     */
    bool run();

    // +-----------------------------------+
    // | FUNCTIONS THAT EXPOSE THE SERVICE |
    // +-----------------------------------+
    /**
     * @brief Exposes core_publish and core_publish_returningid functionalities to external classes
     *
     * @param id Id field. If negative, it assigns the id automatically (RETURNING id)
     * @param name Name field of the message
     * @param created Created field of the message
     * @param deleted Deleted field of the message
     * @param create_revision Create_revision field
     * @param prev_revision Prev_revision field of the message
     * @param lease Lease field of the message
     * @param value Value field of the message
     * @param old_value Old_value field of the message
     * @return int
     */
    std::pair<unsigned long, bool> expose_publish(int id, std::string name,
                                                  unsigned long created, unsigned long deleted,
                                                  unsigned long create_revision, unsigned long prev_revision,
                                                  unsigned long lease,
                                                  std::vector<unsigned char> value, std::vector<unsigned char> old_value);

    /**
     * @brief Exposes core_dispose to external classes
     *
     * @param id Id of the message to dispose
     * @return true, if the message was disposed correctly
     * @return false, if some error happened
     */
    bool expose_dispose(unsigned long id);

    /**
     * @brief Exposes mass dispose for compaction purposes
     *
     * @param param1 Parameter $1 of the query
     * @param param2 Parameter $2 of the query
     * @return QueryStructure::structure_massDisposeCompaction
     */
    QueryStructure::structure_massDisposeCompaction expose_massDisposeCompaction(int param1, int param2);

    /**
     * @brief Exposes queries 1, 4, 9 and 11 to external classes
     *
     * @param query_identifier Identifier of the query (1, 4, 9 or 11)
     * @param limit Maximum number of messages to return
     * @param param1 Parameter $1 of the query
     * @param param2 Parameter $2 of the query
     * @param param3 Parameter $3 of the query
     * @param param4 Parameter $4 of the query
     * @param param5 Parameter $5 of the query
     * @return std::vector<QueryStructure::structure_generic>
     */
    std::vector<QueryStructure::structure_generic> expose_querygeneric(const int query_identifier, int limit,
                                                                       std::string param1, std::string param2,
                                                                       std::string param3, std::string param4, std::string param5);

    /**
     * @brief Exposes query 3
     *
     * @param param1 Parameter $1 of the query
     * @param param2 Parameter $2 of the query
     * @return QueryStructure::structure_query03
     */
    QueryStructure::structure_query03 expose_query03(std::string param1, std::string param2);

    /**
     * @brief Exposes query_compaction (the query for "compact_rev_key")
     *
     * @return unsigned long, id of the row named "compact_rev_key"
     */
    unsigned long expose_querycompactrevision();

    /**
     * @brief Exposes query_max_id
     *
     * @return std::pair<unsigned long, bool>, maximum id in the database
     */
    std::pair<unsigned long, bool> expose_querymaximumid();

    // +-----------------------+
    // | MEASUREMENT FUNCTIONS |
    // +-----------------------+
    /**
     * @brief Prints the measurements stored in the measurement vector
     *
     */
    void print_measurements();

    /**
     * @brief Prints the stored measurement sizes of the messages
     * 
     */
    void print_message_sizes();

    // +----------------------+
    // | WORKAROUND FUNCTIONS |
    // +----------------------+
    /**
     * @brief Used to republish messages when it is detected that another node died. Main topic. Unused
     *  
     * @param msg 
     */
    void republish_message(DatabaseData::Msg msg);

    /**
     * @brief Used to republish messages when it is detected that another node died. MaxId topic
     * 
     * @param msg 
     */
    void republish_message_maxid();

    /**
     * @brief Used to republish messages when it is detected that another node died. Both topic (calls republish_message_maxid)
     * 
     */
    void republish_everything();
};