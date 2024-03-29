/**un()
 * @file Database.cpp
 * @author Saül Abad Copoví
 * @brief Database code
 */
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <set>
#include <iterator>
#include <mutex>
#include <chrono>
#include <x86intrin.h> // For rdtsc
#include <unistd.h>    // For gethostname

#include "Database.hpp"
#include "ListenerDataReader.h"
#include "ListenerDataWriter.h"

using namespace org::eclipse::cyclonedds;

using std::cin;
using std::cout;
using std::endl;
using std::chrono::duration_cast;
using std::chrono::steady_clock;

namespace policy = dds::core::policy;

// Used to detect when the handled signal is triggered
volatile bool finish = false;

// Handler for the sigint signal
void multipurpose_sighandler(int sig)
{
    finish = true;
}

//! MEASUREMENT CODE
//! For latency measurement purposes
// std::map<int, unsigned long> latency_timestamps_maintopic; 
// std::map<int, unsigned long> latency_timestamps_maxidtopic; 
// std::vector<std::string> measurement_vector;                
// std::mutex mutex_timestamps_maintopic;                      
// std::mutex mutex_timestamps_maxidtopic;                     
// std::mutex mutex_measurement_vector;                        
// std::string hostname;                                       
// std::string maintopic_id_sending;                           
// std::string maintopic_id_receiving;                         
// std::string maxidtopic_id_sending;                          
// std::string maxidtopic_id_receiving;                        

// +-----------+
// | VARIABLES |
// +-----------+
//! Indicates if the publisher matches any reader
std::atomic_int matched;
std::atomic_int matched_maxid;
//! Declaration of parameters for QueryConditions
unsigned long parameters_id_;
std::shared_ptr<std::vector<int>> parameters_mass_dispose_compaction_;
std::shared_ptr<std::vector<std::string>> parameters_query01_;
std::shared_ptr<std::vector<std::string>> parameters_query04_;
std::shared_ptr<std::vector<std::string>> parameters_query09_;
std::shared_ptr<std::vector<std::string>> parameters_query11_;
//! Guard to prevent corruption of the maxid topic's content
std::mutex mutex_maxidtopic;
std::mutex mutex_messageById;
std::mutex mutex_massDisposeCompaction;
std::mutex mutex_query01;
std::mutex mutex_query04;
std::mutex mutex_query09_subquery01;
std::mutex mutex_query09_subquery02;
std::mutex mutex_query11;
//! Workaround to resend received messages to get ownership
Database *this_db_;
std::atomic_bool started_republish;
std::set<uint64_t> republished_messages;
std::set<uint64_t> republished_messages_maxid;

// +---------------------------+
// | CONSTRUCTOR AND DESTROYER |
// +---------------------------+
Database::Database(int max_messages)
    : participant_(nullptr),
      publisher_(nullptr), subscriber_(nullptr),
      topic_(nullptr),
      writer_(nullptr), reader_(nullptr),
      done_(false), maximum_messages_(max_messages)
{
}

Database::~Database()
{
    sleep(1);
    // TODO: clean all the database elements
    if (participant_ != nullptr)
    {
    }

    if (publisher_ != nullptr)
    {
    }

    if (subscriber_ != nullptr)
    {
    }

    if (topic_ != nullptr)
    {
    }

    if (writer_ != nullptr)
    {
    }

    if (reader_ != nullptr)
    {
    }

    cout << "=== Database destroyed" << endl;
}

/**
 * @brief Reads the real TSC, as in the host's through rdpmc
 *
 * @return unsigned long TSC value
 */
unsigned long rdtsc_rdpmc()
{
    uint32_t lo, hi;
    __asm__ __volatile__("rdpmc" : "=a"(lo), "=d"(hi) : "c"(0x10000));
    return ((uint64_t)hi << 32) | lo;
}

// +--------------------+
// | LISTENER FUNCTIONS |
// +--------------------+
bool Database::listener_ondataavailable(dds::sub::DataReader<DatabaseData::Msg> &reader)
{
    try
    {
        //! MEASUREMENT CODE
        // unsigned long final = rdtsc_rdpmc();
        auto msgs = reader.select().state(dds::sub::status::InstanceState::alive()).state(dds::sub::status::SampleState::not_read()).read();
        if (msgs.length() > 0)
        {
            for (auto i = msgs.begin(); i < msgs.end(); ++i)
            {
                // Check if the message received has an empty name (it is an invalid message, which means that the writer on the other DB died)
                // Could also check for invalid messages instead, also
                if (i->data().name() == "")
                {
                    // Check if another thread started the republishing
                    if (not started_republish.load())
                    {
                        started_republish.store(true);
                        // Republish the dead messages
                        std::thread thread_republisher(&Database::republish_everything, this_db_);
                        thread_republisher.detach();
                    }
                }
                else if (i->info().valid())
                {
                    DatabaseData::Msg msg = i->data();
                    // cout << "Message received with id " << msg.id() << " and name " << msg.name() << endl;

                    //! MEASUREMENT CODE
                    // {
                    // std::lock_guard<std::mutex> guard(mutex_measurement_vector);
                    // measurement_vector.push_back(maintopic_id_receiving + std::to_string(msg.id()) + "," + std::to_string(final));
                    // }
                }
            }
        }
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        std::cout << "# Timeout encountered" << std::endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in listener (on data available)" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

bool Database::listener_subscriptionmatched(dds::sub::DataReader<DatabaseData::Msg> &reader)
{
    cout << "listener_subscriptionmatched triggered" << endl;
    std::cout << "=== Subscription change" << std::endl;
    try
    {
        if (reader.subscription_matched_status().current_count_change() > 0)
        {
            cout << "== Subscriber matched" << endl;
        }
        else if (reader.subscription_matched_status().current_count_change() < 0)
        {
            cout << "== Subscriber unmatched" << endl;
        }
        else
        {
            cout << "== Subscription: unexpected number" << endl;
        }
        cout << "=== Subscription curent_count: " << reader.subscription_matched_status().current_count_change() << endl;
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        std::cout << "# Timeout encountered" << std::endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in listener (subscription matched)" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

bool Database::listener_publicationmatched(dds::pub::DataWriter<DatabaseData::Msg> &writer)
{
    cout << "listener_publicationmatched triggered" << endl;
    std::cout << "=== Publication change" << std::endl;
    try
    {
        if (writer.publication_matched_status().current_count_change() == 1)
        {
            matched = writer.publication_matched_status().total_count();
            cout << "== Publisher matched" << endl;
        }
        else if (writer.publication_matched_status().current_count_change() == -1)
        {
            matched = writer.publication_matched_status().total_count();
            cout << "== Publisher unmatched" << endl;
        }
        else
        {
            cout << "== Publication: wrong number?" << endl;
        }
        cout << "=== Publication current_count: " << writer.publication_matched_status().current_count_change() << endl;
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        std::cout << "# Timeout encountered" << std::endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in listener (publication matched)" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

bool Database::listener_ondataavailablemaxid(dds::sub::DataReader<IdentifierTracker::MaxIdMsg> &reader)
{
    try
    {
        //! MEASUREMENT CODE
        // unsigned long final = rdtsc_rdpmc();
        auto msgs = reader.select().state(dds::sub::status::InstanceState::alive()).state(dds::sub::status::SampleState::not_read()).read();
        if (msgs.length() > 0)
        {
            for (auto i = msgs.begin(); i < msgs.end(); ++i)
            {
                if (i->info().valid())
                {
                    IdentifierTracker::MaxIdMsg msg = i->data();
                    //! MEASUREMENT CODE
                    // {
                    // std::lock_guard<std::mutex> guard(mutex_measurement_vector);
                    // measurement_vector.push_back(maxidtopic_id_receiving + std::to_string(msg.maxid()) + "," + std::to_string(final));
                    // }
                }
            }
        }
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        std::cout << "# Timeout encountered" << std::endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in listener (on data available)" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

bool Database::listener_subscriptionmatchedmaxid(dds::sub::DataReader<IdentifierTracker::MaxIdMsg> &reader)
{
    cout << "listener_subscriptionmatchedmaxid triggered" << endl;
    std::cout << "=== Subscription change" << std::endl;
    try
    {
        if (reader.subscription_matched_status().current_count_change() > 0)
        {
            cout << "== Subscriber matched" << endl;
        }
        else if (reader.subscription_matched_status().current_count_change() < 0)
        {
            cout << "== Subscriber unmatched" << endl;
        }
        else
        {
            cout << "== Subscription: unexpected number" << endl;
        }
        cout << "=== Subscription curent_count: " << reader.subscription_matched_status().current_count_change() << endl;
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        std::cout << "# Timeout encountered" << std::endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in listener (subscription matched)" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

bool Database::listener_publicationmatchedmaxid(dds::pub::DataWriter<IdentifierTracker::MaxIdMsg> &writer)
{
    cout << "listener_publicationmatchedmaxid triggered" << endl;
    std::cout << "=== Publication change" << std::endl;
    try
    {
        if (writer.publication_matched_status().current_count_change() == 1)
        {
            matched_maxid = writer.publication_matched_status().total_count();
            cout << "== Publisher matched" << endl;
        }
        else if (writer.publication_matched_status().current_count_change() == -1)
        {
            matched_maxid = writer.publication_matched_status().total_count();
            cout << "== Publisher unmatched" << endl;
        }
        else
        {
            cout << "== Publication: wrong number?" << endl;
        }
        cout << "=== Publication current_count: " << writer.publication_matched_status().current_count_change() << endl;
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        std::cout << "# Timeout encountered" << std::endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in listener (publication matched)" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

// +-------------------+
// | WAITSET FUNCTIONS |
// +-------------------+
// This code is used to match the readers and writers when they are first created. Main topic only
bool Database::match_readers_and_writers(dds::pub::DataWriter<DatabaseData::Msg> &wr, dds::sub::DataReader<DatabaseData::Msg> &rd)
{
    // Create waitset
    dds::core::cond::WaitSet waitset;
    // Create status conditions that can trigger the waitset when a change is detected
    dds::core::cond::StatusCondition statuscondition_writer(wr), statuscondition_reader(rd);
    statuscondition_writer.enabled_statuses(dds::core::status::StatusMask::publication_matched());
    statuscondition_reader.enabled_statuses(dds::core::status::StatusMask::subscription_matched());
    // Attach the waitset to the conditions
    waitset.attach_condition(statuscondition_writer);
    waitset.attach_condition(statuscondition_reader);

    std::cout << "=== Matching readers and writers" << std::endl;
    try
    {
        cout << "Statuses are: " << wr.publication_matched_status().current_count() << " and " << rd.subscription_matched_status().current_count() << endl;
        while (wr.publication_matched_status().current_count() == 0 or rd.subscription_matched_status().current_count() == 0)
        {
            // The waitset will wait infinitely for at least one of the conditions previously specified
            auto conditions = waitset.wait(dds::core::Duration::infinite());
            // Dettach the conditions once they have matched.
            for (const auto &c : conditions)
            {
                if (c == statuscondition_writer)
                    waitset.detach_condition(statuscondition_writer);
                else if (c == statuscondition_reader)
                    waitset.detach_condition(statuscondition_reader);
            }
        }
        cout << "=== Matching happened! (DDS DB)" << endl;
        matched = wr.publication_matched_status().current_count();
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        cout << "# Timeout error which should never have happened" << endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in waitset" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

// This code is used to match the readers and writers when they are first created. MaxId topic only
bool Database::match_readers_and_writers_maxid(dds::pub::DataWriter<IdentifierTracker::MaxIdMsg> &wr, dds::sub::DataReader<IdentifierTracker::MaxIdMsg> &rd)
{
    // Create waitset
    dds::core::cond::WaitSet waitset;
    // Create status conditions that can trigger the waitset when a change is detected
    dds::core::cond::StatusCondition statuscondition_writer(wr), statuscondition_reader(rd);
    statuscondition_writer.enabled_statuses(dds::core::status::StatusMask::publication_matched());
    statuscondition_reader.enabled_statuses(dds::core::status::StatusMask::subscription_matched());
    // Attach the waitset to the conditions
    waitset.attach_condition(statuscondition_writer);
    waitset.attach_condition(statuscondition_reader);

    std::cout << "=== Matching readers and writers" << std::endl;
    try
    {
        cout << "Statuses are: " << wr.publication_matched_status().current_count() << " and " << rd.subscription_matched_status().current_count() << endl;
        while (wr.publication_matched_status().current_count() == 0 or rd.subscription_matched_status().current_count() == 0)
        {
            // The waitset will wait infinitely for at least one of the conditions previously specified
            auto conditions = waitset.wait(dds::core::Duration::infinite());
            // Dettach the conditions once they have matched.
            for (const auto &c : conditions)
            {
                if (c == statuscondition_writer)
                    waitset.detach_condition(statuscondition_writer);
                else if (c == statuscondition_reader)
                    waitset.detach_condition(statuscondition_reader);
            }
        }
        cout << "=== Matching happened! (MaxId)" << endl;
        matched_maxid = wr.publication_matched_status().current_count();
        return true;
    }
    catch (const dds::core::TimeoutError &)
    {
        cout << "# Timeout error which should never have happened" << endl;
        return false;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in waitset" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
}

// +----------------------+
// | RUN & INIT FUNCTIONS |
// +----------------------+
// Initialises the DDS components that create the Database
bool Database::init()
{
    cout << "=== Initialising Database" << endl;
    try
    {
        /**
         * @brief Define the QoS of the Topic, DataWriter and DataReader. Ensure they are all the same by copying the values.
         * For a database, use:
         * 1. Transient Durability: Messages are kept in memory
         * 2. Reliable Reliability: All messages are transmitted
         * 3. Keep-All History: No messages are removed from the history
         * 4. By Source Timestamp order: The order of messages is the order in which they have been sent
         */
        dds::topic::qos::TopicQos qos_topic;
        dds::pub::qos::DataWriterQos qos_datawriter;
        dds::sub::qos::DataReaderQos qos_datareader;
        dds::topic::qos::TopicQos qos_topic_maxid;
        dds::pub::qos::DataWriterQos qos_datawriter_maxid;
        dds::sub::qos::DataReaderQos qos_datareader_maxid;
        qos_topic << policy::Reliability::Reliable(dds::core::Duration::from_secs(0.4))
                  << policy::Durability::TransientLocal()
                  << policy::DurabilityService(dds::core::Duration(0, 0), policy::HistoryKind::Type::KEEP_ALL, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                  << policy::ResourceLimits(DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                  << policy::DestinationOrder(policy::DestinationOrderKind::Type::BY_SOURCE_TIMESTAMP);
        qos_datawriter << policy::Reliability::Reliable(dds::core::Duration::from_secs(0.4))
                       << policy::Durability::TransientLocal()
                       << policy::DurabilityService(dds::core::Duration(0, 0), policy::HistoryKind::Type::KEEP_ALL, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                       << policy::ResourceLimits(DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                       << policy::DestinationOrder(policy::DestinationOrderKind::Type::BY_SOURCE_TIMESTAMP)
                       << policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
        qos_datareader << policy::Reliability::Reliable(dds::core::Duration::from_secs(0.4))
                       << policy::Durability::TransientLocal()
                       << policy::ResourceLimits(DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                       << policy::DestinationOrder(policy::DestinationOrderKind::Type::BY_SOURCE_TIMESTAMP)
                       << policy::ReaderDataLifecycle::NoAutoPurgeDisposedSamples();

        // The max id topic only needs to keep the last message, as it only should have a maximum id at all times
        qos_topic_maxid << policy::Reliability::Reliable(dds::core::Duration::from_secs(0.4))
                        << policy::Durability::TransientLocal()
                        << policy::DurabilityService(dds::core::Duration(0, 0), policy::HistoryKind::KEEP_LAST, 4, 4, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                        << policy::DestinationOrder(policy::DestinationOrderKind::Type::BY_SOURCE_TIMESTAMP)
                        << policy::ResourceLimits(DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                        << policy::History(policy::HistoryKind::KEEP_LAST, 1);
        qos_datawriter_maxid << policy::Reliability::Reliable(dds::core::Duration::from_secs(0.4))
                             << policy::Durability::Volatile()
                             << policy::DurabilityService(dds::core::Duration(0, 0), policy::HistoryKind::KEEP_LAST, 4, 4, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                             << policy::ResourceLimits(DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                             << policy::DestinationOrder(policy::DestinationOrderKind::Type::BY_SOURCE_TIMESTAMP)
                             << policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances()
                             << policy::History(policy::HistoryKind::KEEP_LAST, 1)
                             << policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
        qos_datareader_maxid << policy::Reliability::Reliable(dds::core::Duration::from_secs(0.4))
                             << policy::Durability::Volatile()
                             << policy::ResourceLimits(DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED, DDS_LENGTH_UNLIMITED)
                             << policy::DestinationOrder(policy::DestinationOrderKind::Type::BY_RECEPTION_TIMESTAMP)
                             << policy::ReaderDataLifecycle::AutoPurgeDisposedSamples(dds::core::Duration(0, 0));

        // Create the participant
        auto participant = std::make_shared<dds::domain::DomainParticipant>(domain::default_id());

        // Create the topic
        auto topic = std::make_shared<dds::topic::Topic<DatabaseData::Msg>>(*participant, "DatabaseDataTopic", qos_topic);

        // Create the publisher
        auto publisher = std::make_shared<dds::pub::Publisher>(*participant);

        // Create the subscriber
        auto subscriber = std::make_shared<dds::sub::Subscriber>(*participant);

        // Create the DataWriter
        ListenerDataWriter dwlistener(&Database::listener_publicationmatched);
        auto writer =
            std::make_shared<dds::pub::DataWriter<DatabaseData::Msg>>(*publisher, *topic, qos_datawriter, &dwlistener, dds::core::status::StatusMask::publication_matched());

        // Create the DataReader
        ListenerDataReader drlistener(&Database::listener_ondataavailable, &Database::listener_subscriptionmatched);
        auto reader =
            std::make_shared<dds::sub::DataReader<DatabaseData::Msg>>(*subscriber, *topic, qos_datareader, &drlistener, dds::core::status::StatusMask::all());

        /**
         * @brief Initialisation of necessary variables for the QueryConditions
         * 1. Get the reader handle
         * 2. Initialise the size of the parameters vectors
         * The QueryConditions themselves are created in the query function or else the messages are not filtered correctly
         */
        auto reader_handle = std::make_shared<dds_entity_t>((*reader)->get_ddsc_entity());

        // Parameters for the mass dispose for compaction. Additional field for the second subquery.
        auto parameters_mass_dispose_compaction = std::make_shared<std::vector<int>>(3, 0);
        // Parameters for Query01
        auto parameters_query01 = std::make_shared<std::vector<std::string>>(2, "");
        // Parameters for Query04
        auto parameters_query04 = std::make_shared<std::vector<std::string>>(2, "");
        // Parameters for Query09. Additional field for the second subquery.
        auto parameters_query09 = std::make_shared<std::vector<std::string>>(6, "");
        // Parameters for Query11
        auto parameters_query11 = std::make_shared<std::vector<std::string>>(3, "");

        //! Bind the everything with the pointers
        participant_ = participant;
        topic_ = topic;
        publisher_ = publisher;
        subscriber_ = subscriber;
        writer_ = writer;
        reader_ = reader;
        reader_handle_ = reader_handle;
        parameters_mass_dispose_compaction_ = parameters_mass_dispose_compaction;
        parameters_query01_ = parameters_query01;
        parameters_query04_ = parameters_query04;
        parameters_query09_ = parameters_query09;
        parameters_query11_ = parameters_query11;

        // Do the first matching of readers and writers
        (void)match_readers_and_writers(*writer_, *reader_);

        // Create the participant
        auto participant_maxid = std::make_shared<dds::domain::DomainParticipant>(domain::default_id());
        // Create the topic
        auto topic_maxid = std::make_shared<dds::topic::Topic<IdentifierTracker::MaxIdMsg>>(*participant_maxid, "IdentifierTrackerTopic", qos_topic_maxid);
        // Create the publisher
        auto publisher_maxid = std::make_shared<dds::pub::Publisher>(*participant_maxid);
        // Create subscriber
        auto subscriber_maxid = std::make_shared<dds::sub::Subscriber>(*participant_maxid);
        // Create DataWriter
        ListenerDataWriterMaxId dwlistenermaxid(&Database::listener_publicationmatchedmaxid);
        auto writer_maxid =
            std::make_shared<dds::pub::DataWriter<IdentifierTracker::MaxIdMsg>>(*publisher_maxid, *topic_maxid, qos_datawriter_maxid, &dwlistenermaxid, dds::core::status::StatusMask::publication_matched());
        // Create DataReader
        ListenerDataReaderMaxId drlistenermaxid(&Database::listener_ondataavailablemaxid, &Database::listener_subscriptionmatchedmaxid);
        auto reader_maxid =
            std::make_shared<dds::sub::DataReader<IdentifierTracker::MaxIdMsg>>(*subscriber_maxid, *topic_maxid, qos_datareader_maxid, &drlistenermaxid, dds::core::status::StatusMask::all());

        // Create the reader handle also for the MaxId
        auto reader_maxid_handle = std::make_shared<dds_entity_t>((*reader_maxid)->get_ddsc_entity());

        // Link the entities with their pointer
        participant_maxid_ = participant_maxid;
        topic_maxid_ = topic_maxid;
        publisher_maxid_ = publisher_maxid;
        subscriber_maxid_ = subscriber_maxid;
        writer_maxid_ = writer_maxid;
        reader_maxid_ = reader_maxid;
        reader_maxid_handle_ = reader_maxid_handle;

        //! Initialise values for the TRANSIENT workaround
        this_db_ = this;
        started_republish.store(false);

        (void)match_readers_and_writers_maxid(*writer_maxid_, *reader_maxid_);

        cout << "=== Finished initialising Database" << endl;

        //! MEASUREMENT CODE
        // Used to stamp the messages with the name of the Node
        // char hn[10];
        // gethostname(hn, 10);
        // hn[0] -= 32;
        // hostname = hn;
        // maintopic_id_sending = "MainTopic" + hostname + "Send,";
        // maxidtopic_id_sending = "MaxIdTopic" + hostname + "Send,";
        // maintopic_id_receiving = "MainTopic" + hostname + "Receive,";
        // maxidtopic_id_receiving = "MaxIdTopic" + hostname + "Receive,";

        run();
    }
    catch (const dds::core::Exception &e)
    {
        std::cerr << "=== Exception: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "### Generic error in init" << std::endl;
        std::exception_ptr e = std::current_exception();
        utils_exception_handler(e);
        return false;
    }
    return true;
}

bool Database::run()
{
    // Binds SIGINT to the function
    signal(SIGINT, multipurpose_sighandler);
    cout << "=== Running Database" << endl;

    while (!Database::done_)
    {
        if (finish)
        {
            // --> Database content inspections. 
            // Dumps all the content of the DB into the terminal upon SIGINT reception.
            // The DB needs to be killed using SIGTERM or similar
            std::thread printer(&Database::TESTING_print_all_history, this);
            printer.join();
            std::thread printer_maxid(&Database::TESTING_print_all_history_maxid, this);
            printer_maxid.join();
            finish = false;

            //! MEASUREMENT CODE
            // --> For size of messages measurements
            // std::thread printer(&Database::print_message_sizes, this);
            // printer.join();
            // raise(SIGTERM);

            //! MEASUREMENT CODE
            // --> For metrics measurements
            // std::thread printer(&Database::print_measurements, this);
            // printer.join();
            // raise(SIGTERM);
        }
    }

    return true;
}

// +-----------+
// | UTILITIES |
// +-----------+
// To get better info about whatever exception is sent
void Database::utils_exception_handler(std::exception_ptr exception_pointer)
{
    try
    {
        if (exception_pointer)
            std::rethrow_exception(exception_pointer);
    }
    catch (const std::exception &e)
    {
        std::cout << "## Exception occurred!" << endl
                  << "# Exception is: " << endl
                  << e.what() << endl;
    }
}

std::string Database::utils_remove_percentage(std::string s)
{
    s.erase(std::remove(s.begin(), s.end(), '%'), s.end());
    return s;
}

bool Database::utils_has_percentage(std::string s)
{
    return s.find("%") != std::string::npos;
}

// +----------------+
// | CORE FUNCTIONS |
// +----------------+
std::pair<unsigned long, bool> Database::core_publish(unsigned long id, std::string name, unsigned long created, unsigned long deleted,
                                                      unsigned long create_revision, unsigned long prev_revision,
                                                      unsigned long lease, std::vector<unsigned char> value, std::vector<unsigned char> old_value)
{
    std::pair<unsigned long, bool> result(0, true);
    if (matched > 0)
    {
        DatabaseData::Msg msg;
        msg.id() = id;
        msg.name() = name;
        msg.created() = created;
        msg.deleted() = deleted;
        msg.create_revision() = create_revision;
        msg.prev_revision() = prev_revision;
        msg.lease() = lease;
        msg.value() = value;
        msg.old_value() = old_value;
        try
        {
            unsigned long initial = rdtsc_rdpmc();
            writer_->write(msg);
            // { // TODO: Comment after latency measurements
            //     std::lock_guard<std::mutex> guard(mutex_measurement_vector);
            //     measurement_vector.push_back(maintopic_id_sending + std::to_string(id) + "," + std::to_string(initial));
            // }
            return result;
        }
        catch (const dds::core::Exception &e)
        {
            std::cout << "# Error: \"" << e.what() << std::endl;
            result.second = false;
            return result;
        }
    }
    cout << "==== No reader is available (DDS DB Specified Id)" << endl;
    result.second = false;
    return result;
}

std::pair<unsigned long, bool> Database::core_publish_returningid(std::string name, unsigned long created, unsigned long deleted,
                                                                  unsigned long create_revision, unsigned long prev_revision,
                                                                  unsigned long lease, std::vector<unsigned char> value, std::vector<unsigned char> old_value)
{
    std::pair<unsigned long, bool> result(0, false);
    // Get the maximum id. New id should be the maximum + 1
    result = query_maxid_and_replace();

    DatabaseData::Msg msg;
    msg.id() = result.first;
    msg.name() = name;
    msg.created() = created;
    msg.deleted() = deleted;
    msg.create_revision() = create_revision;
    msg.prev_revision() = prev_revision;
    msg.lease() = lease;
    msg.value() = value;
    msg.old_value() = old_value;
    try
    {
        unsigned long initial = rdtsc_rdpmc();
        writer_->write(msg);
        // { // TODO: Comment after latency measurements
        //     std::lock_guard<std::mutex> guard(mutex_measurement_vector);
        //     measurement_vector.push_back(maintopic_id_sending + std::to_string(result.first) + "," + std::to_string(initial));
        // }
        return result;
    }
    catch (const dds::core::Exception &e)
    {
        std::cout << "# Error: \"" << e.what() << std::endl;
        return result;
    }
    cout << "==== No reader is available (DDS DB Returning Id)" << endl;
    return result;
}

bool Database::core_dispose(DatabaseData::Msg data)
{
    writer_->dispose_instance(data);
    writer_->unregister_instance(data);
    // TODO: consider taking the messages after disposing them with:
    // TODO: Check whether the disposition actually worked and return the proper bool
    return true;
}

// +-----------------+
// | QUERY FUNCTIONS |
// +-----------------+
QueryStructure::structure_massDisposeCompaction Database::query_mass_dispose_compaction(int param1, int param2)
{
    // Create the result structure
    QueryStructure::structure_massDisposeCompaction result(false, 0);
    int samples_received = 0;

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    {
        std::lock_guard<std::mutex> guard(mutex_massDisposeCompaction);
        (*parameters_mass_dispose_compaction_)[0] = param1;
        (*parameters_mass_dispose_compaction_)[1] = param2;

        // Create the QueryCondition considering only ALIVE messages
        dds_entity_t queryCondition_massDisposeCompaction =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(filter_mass_dispose_compaction_subquery01));

        samples_received = dds_read(queryCondition_massDisposeCompaction, msgs_pointers, msgs_infos, msgs_number, msgs_number);
        dds_delete(queryCondition_massDisposeCompaction);

        for (int i = 0; i < samples_received; ++i)
        {
            if (msgs_infos[i].valid_data)
            {

                (*parameters_mass_dispose_compaction_)[2] = msgs_data[i].id();
                dds_entity_t queryCondition_massDisposeCompaction_subquery02 =
                    dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(filter_mass_dispose_compaction_subquery02));
                int samples_received_subquery02 = dds_read(queryCondition_massDisposeCompaction_subquery02, msgs_pointers, msgs_infos, msgs_number, msgs_number);
                // for (int j = 0; j < )
                // TODO: Implement the compaction mechanism
            }
        }
    }

    // If nothing was read, return rows_disposed = 0
    if (samples_received < 1)
    {
        result.success = true;
        return result;
    }

    for (int i = 0; i < samples_received; ++i)
    {
        if (msgs_infos[i].valid_data)
        {
            TESTING_print_message(msgs_data[i]); // TODO: Delete this
            // core_dispose(msgs_data[i]); // TODO: Uncomment this
        }
    }
    cout << "Samples_received = " << samples_received << endl;

    return result;
}

DatabaseData::Msg Database::query_compaction()
{
    // Create the result structure
    DatabaseData::Msg result(0, "empty", 0, 0, 0, 0, 0, std::vector<uint8_t>(), std::vector<uint8_t>());
    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    // Create the QueryCondition considering only ALIVE messages
    dds_entity_t queryCondition_compaction =
        dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(filter_query_compaction));

    int samples_received = dds_read(queryCondition_compaction, msgs_pointers, msgs_infos, msgs_number, msgs_number);
    dds_delete(queryCondition_compaction);

    // Get the message with the id
    for (int i = 0; i < samples_received; ++i)
    {
        if (msgs_infos[i].valid_data)
        {
            result = msgs_data[i];
            break;
        }
    }

    // In case there's not a 'compact_rev_key', an empty message is returned
    // If such thing happens, Kine has not started properly, since that's the one publishing this entry
    return result;
}

DatabaseData::Msg Database::query_find_message_by_id(unsigned long id)
{
    // Create the result structure
    DatabaseData::Msg result(0, "empty", 0, 0, 0, 0, 0, std::vector<uint8_t>(), std::vector<uint8_t>());
    int samples_received = 0;

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    // Define the id to find
    {
        std::lock_guard<std::mutex> guard(mutex_messageById);
        parameters_id_ = id;

        // Create the QueryCondition considering only ALIVE messages
        dds_entity_t queryCondition_id =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(filter_query_find_message_by_id));

        samples_received = dds_read(queryCondition_id, msgs_pointers, msgs_infos, msgs_number, msgs_number);
        dds_delete(queryCondition_id);
    }
    // If no message with that id has been found, return an empty message
    if (samples_received < 1)
        return result;

    // Get the message with the id
    for (int i = 0; i < samples_received; ++i)
    {
        if (msgs_infos[i].valid_data)
        {
            result = msgs_data[i];
            break;
        }
    }

    return result;
}

std::pair<unsigned long, bool> Database::query_maxid()
{
    /**
     * gRPC produces multithreading, so more than one thread could access this function at the same time,
     * If that happens, in combination with takes produced by query_maxid_an_replace, it is possible to
     * read just after the take has been performed, thus returning a 0 instead of the real maxid
     */
    std::lock_guard<std::mutex> guard(mutex_maxidtopic);

    // Create an empty result
    std::pair<unsigned long, bool> result(0, false);

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    IdentifierTracker::MaxIdMsg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    // Using this as some sort of workaround: there is no way to limit the number of different keys
    // Maybe could be solved by not using primary keys (but this way is faster)
    // All messages are taken, then the maxid is republished again
    int samples_received = dds_read((*reader_maxid_handle_), msgs_pointers, msgs_infos, msgs_number, msgs_number);

    // Get the message with the id
    for (int i = 0; i < samples_received; ++i)
    {
        if (msgs_infos[i].valid_data)
        {
            if (msgs_data[i].maxid() > result.first)
            {
                result.first = msgs_data[i].maxid();
                result.second = true;
            }
        }
    }
    return result;
}

std::pair<unsigned long, bool> Database::query_maxid_and_replace()
{
    /**
     * gRPC produces multithreading, so more than one thread could access this function at the same time,
     * If that happens, the first thread takes the maxid from the MaxId topic, thus leaving the topic empty.
     * The other topics will then put a 1 as maxid and the real maximum id will be no more.
     */
    std::lock_guard<std::mutex> guard(mutex_maxidtopic);

    // Create an empty result
    std::pair<unsigned long, bool> result(0, false);

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    IdentifierTracker::MaxIdMsg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    int samples_received = dds_take((*reader_maxid_handle_), msgs_pointers, msgs_infos, msgs_number, msgs_number);

    // Get the message with the id
    for (int i = 0; i < samples_received; ++i)
    {
        if (msgs_infos[i].valid_data)
        {
            if (msgs_data[i].maxid() > result.first)
                result.first = msgs_data[i].maxid();
        }
    }

    if (matched_maxid > 0)
    {
        ++result.first;
        try
        {
            IdentifierTracker::MaxIdMsg msg;
            msg.maxid() = result.first;
            // unsigned long initial = rdtsc_rdpmc(); // TODO: Comment after latency measurements
            writer_maxid_->write(msg);
            // { // TODO: Comment after latency measurements
            //     std::lock_guard<std::mutex> guard(mutex_measurement_vector);
            //     measurement_vector.push_back(maxidtopic_id_sending + std::to_string(msg.maxid()) + "," + std::to_string(initial));
            // }
        }
        catch (const dds::core::Exception &e)
        {
            std::cout << "# Error: \"" << e.what() << std::endl
                      << std::flush;
            result.second = false;
            return result;
        }
        return result;
    }
    cout << "==== No reader is available (MaxId)" << endl;
    result.second = false;
    return result;
}

void Database::query_find_common_values(unsigned long *max_id, unsigned long *compaction_prev_revision)
{
    // TODO: Check if !max_id.second, that is an error (exception or no writer matched)
    *max_id = query_maxid().first;
    *compaction_prev_revision = query_compaction().prev_revision();
}

int Database::query_count_all()
{
    auto msgs = reader_->select().max_samples(DDS_LENGTH_UNLIMITED).read();
    return msgs.length();
}

std::vector<QueryStructure::structure_generic> Database::query_common_subquery(
    int iterations, dds_sample_info_t *msgs_infos, DatabaseData::Msg *msgs_data, int max_id, int compaction_prev_revision, std::string t_f)
{
    std::map<std::string, std::pair<QueryStructure::structure_generic, bool>> result_map;
    std::vector<QueryStructure::structure_generic> result;

    for (int i = 0; i < iterations; ++i)
    {
        if ((msgs_infos + i)->valid_data)
        {
            int current_id = (msgs_data + i)->id();

            // Create the QueryStructure::structure_generic with its content
            QueryStructure::structure_generic tmp(max_id, compaction_prev_revision,
                                                  (msgs_data + i)->id(), (msgs_data + i)->name(),
                                                  (msgs_data + i)->created(), (msgs_data + i)->deleted(),
                                                  (msgs_data + i)->create_revision(), (msgs_data + i)->prev_revision(),
                                                  (msgs_data + i)->lease(),
                                                  (msgs_data + i)->value(), (msgs_data + i)->old_value());

            // Create the pair of results according to Subquery03
            std::pair<QueryStructure::structure_generic, bool> content = std::make_pair(tmp, true);

            // Using 'f' implies that messages with a deleted of 1 (deleted != 0) will not be considered. Remove them
            if (t_f == "f")
            {
                if ((msgs_data + i)->deleted() != 0)
                {
                    content.second = false;
                }
            }

            // Putting elements in the map using their name as key, in order to create the equivalent of an SQL GROUP BY name
            auto const return_result = result_map.insert(std::make_pair((msgs_data + i)->name(), content));

            // The previously executed insert has returned a pair, where first is a pointer to the newly inserted element or an element with equivalent key
            // Second indicates if a new element was inserted (true) or if an equivalent key already existed (false)
            // Therefore, the if only is executed when no new element was inserted
            if (not return_result.second)
            {
                // Comparing the current id with the id of the message that is in the map
                if (current_id > return_result.first->second.first.theid)
                {
                    // Substitute the previously existing message with the one with bigger id, keeping all the fields (prev_revision and values may differ from the old one)
                    return_result.first->second.first = tmp;
                }
            }
        }
    }

    // Put the result in a vector, sort it according to id ASC and return
    for (auto elem : result_map)
    {
        if (elem.second.second)
        {
            result.push_back(elem.second.first);
        }
    }

    // Perform the SORT BY id ASC and return
    std::sort(result.begin(), result.end());
    return result;
}

/**
 * Queries can have a LIMIT and an ORDER BY. However, this poses a problem:
 * In SQL, all rows are ordered as they are inserted.
 * In DDS, however, that's not the case. They are ordered as they are received.
 * Because of this, instead of using the limit of retreived messages that can be used in DDS reads, all messages are read and then sorted, before
 * resizing the result vector to fit the specified limit. This way, the messages read are in par with SQL. In queries 4, 9 and 11, the sort is performed
 * within query_common_subquery, while in 1 is done explicitely before returning the result.
 *
 * If that was not the case, this could have been used to apply the limit in the read itself:
 *  int msgs_number = limit < 0 ? maximum_messages_ : limit;
 */

std::vector<QueryStructure::structure_generic> Database::query_01(int limit, std::vector<std::string> parameter_vector)
{
    // Create the result vector
    std::vector<QueryStructure::structure_generic> result;
    int samples_received = 0;

    // Get the maximum id and the prev_revision of the compaction
    unsigned long max_id, compaction_prev_revision;
    query_find_common_values(&max_id, &compaction_prev_revision);

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    bool percentage = utils_has_percentage(parameter_vector[0]);

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    // Perform the read within a protected mutex to avoid concurrent modifications to the parameter vectors
    {
        std::lock_guard<std::mutex> guard(mutex_query01);
        // Set the parameters for the filter
        (*parameters_query01_)[0] = percentage ? utils_remove_percentage(parameter_vector[0]) : parameter_vector[0];
        (*parameters_query01_)[1] = parameter_vector[1];

        dds_entity_t queryCondition_query01 =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE,
                                      dds_querycondition_filter_fn(percentage ? Database::filter_query01_percentage : Database::filter_query01));

        // Read using the QueryCondition, keep the return value to populate the result
        samples_received = dds_read(queryCondition_query01, msgs_pointers, msgs_infos, maximum_messages_, maximum_messages_);
        dds_delete(queryCondition_query01);
    }

    if (samples_received > 0)
    {
        // Fill the result vector
        for (int i = 0; i < samples_received; ++i)
        {
            if (msgs_infos[i].valid_data)
            {
                QueryStructure::structure_generic tmp(max_id, compaction_prev_revision,
                                                      msgs_data[i].id(), msgs_data[i].name(),
                                                      msgs_data[i].created(), msgs_data[i].deleted(),
                                                      msgs_data[i].create_revision(), msgs_data[i].prev_revision(),
                                                      msgs_data[i].lease(),
                                                      msgs_data[i].value(), msgs_data[i].old_value());
                result.push_back(tmp);
            }
        }

        // Order the result vector on the id and apply the limit to ensure the same order and result as in SQL
        std::sort(result.begin(), result.end());
        if (limit > 0 and result.size() > limit)
            result.resize(limit);
    }
    return result;
}

QueryStructure::structure_query03 Database::query_03(std::vector<std::string> parameter_vector)
{
    // This query returns two things:
    // 1. The maximum id
    // 2. The count of the rows returned by a Query04 query
    // Query03 only returns one row
    QueryStructure::structure_query03 result;

    // Get the maximum id
    result.id = query_maxid().first;

    // Perform a Query04 and count the messages returned
    result.count = query_04(maximum_messages_, parameter_vector).size();

    return result;
}

std::vector<QueryStructure::structure_generic> Database::query_04(int limit, std::vector<std::string> parameter_vector)
{
    // This query is divided in 2 subqueries:
    // 1. Subquery01: WHERE name LIKE $1 GROUP BY name
    // 2. Subquery02: WHERE deleted = 0 or $2 ($2 is 't' or 'f', only)
    // Due to the 'f' or 't' in Subquery02, only messages with deleted = 0 are considered when 'f', or all messages returned by Subquery01 are considered when 't'.
    // Regardless, everything can be implemented during the evaluation of the results from Subquery01

    // Create the result vectors
    std::vector<QueryStructure::structure_generic> result;
    // Create the result map for the subqueries. Using map to facilitate the GROUP BY implementation
    std::map<std::string, std::pair<QueryStructure::structure_generic, bool>> result_subquery01; // The second bool indicates whether the result is valid
    // Number of results received by the read
    int samples_received_subquery01 = 0;
    std::string t_f = "f"; // Last parameter of the query

    // Get the maximum id and the prev_revision of the compaction
    unsigned long max_id, compaction_prev_revision;
    query_find_common_values(&max_id, &compaction_prev_revision);

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data_subquery01[msgs_number];
    void *msgs_pointers_subquery01[msgs_number];
    dds_sample_info_t msgs_infos_subquery01[msgs_number];

    // Make the pointers point to the data for both subqueries
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers_subquery01[i] = &msgs_data_subquery01[i];
    }

    // Essentially, the process followed is:
    // 0. Prepare parameters and create the QueryCondition
    // 1. Perform subquery01
    // 3. Merge the results performing a join on the id while evaluating results from Subquery01
    bool percentage = utils_has_percentage(parameter_vector[0]);

    // 0.0. Prepare parameters for the query. Protected to avoid multithread issues
    {
        std::lock_guard<std::mutex> guard(mutex_query04);

        (*parameters_query04_)[0] = percentage ? utils_remove_percentage(parameter_vector[0]) : parameter_vector[0];
        t_f = parameter_vector[1];

        // 0.1. Create the QueryConditions
        dds_entity_t queryCondition_query04_subquery01 =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(percentage ? filter_query04_subquery01_percentage : filter_query04_subquery01));

        // 1. Perform subquery01
        samples_received_subquery01 = dds_read(queryCondition_query04_subquery01, msgs_pointers_subquery01, msgs_infos_subquery01, maximum_messages_, maximum_messages_);
        dds_delete(queryCondition_query04_subquery01);
    }

    // Perform the GROUP BY, the JOIN and ORDER BY id ASC. No need to do so if nothing was read.
    if (samples_received_subquery01 > 0)
    {
        result = query_common_subquery(samples_received_subquery01, &msgs_infos_subquery01[0], &msgs_data_subquery01[0],
                                       max_id, compaction_prev_revision, t_f);

        // Apply the limit to ensure the same order and result as in SQL
        if (limit > 0 and result.size() > limit)
            result.resize(limit);
    }

    return result;
}

std::vector<QueryStructure::structure_generic> Database::query_09(int limit, std::vector<std::string> parameter_vector)
{
    // Create the result vectors
    std::vector<QueryStructure::structure_generic> result;
    // Only the maximum id is considered, so only one result is expected
    QueryStructure::structure_generic result_subquery01;
    // Create the result map for the subqueries. Using map to facilitate the GROUP BY implementation
    std::map<std::string, std::pair<QueryStructure::structure_generic, bool>> result_subquery02;
    // Number of messages read for both subqueries
    int samples_received_subquery01 = 0;
    int samples_received_subquery02 = 0;
    std::string t_f = "f"; // Last parameter of the query

    // Get the maximum id and the prev_revision of the compaction
    unsigned long max_id, compaction_prev_revision;
    query_find_common_values(&max_id, &compaction_prev_revision);

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data_subquery01[msgs_number];
    DatabaseData::Msg msgs_data_subquery02[msgs_number];
    void *msgs_pointers_subquery01[msgs_number];
    void *msgs_pointers_subquery02[msgs_number];
    dds_sample_info_t msgs_infos_subquery01[msgs_number];
    dds_sample_info_t msgs_infos_subquery02[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers_subquery01[i] = &msgs_data_subquery01[i];
        msgs_pointers_subquery02[i] = &msgs_data_subquery02[i];
    }

    bool percentage = utils_has_percentage(parameter_vector[0]);

    // Essentially, the process followed is:
    // 0. Prepare parameters and create the QueryCondition for subquery01
    // 1. Perform Subquery01: name = $3 AND id <= $4
    // 2. Prepare the parameters for subquery02 (the result from subquery01) and create the QueryCondition.
    // 3. Perform Subquery02: name LIKE $1 AND id <= $2 AND id > [Subquery01 result]
    // 4. Perform the GROUP BY name on the results of Subquery02 in case anything was returned

    {
        std::lock_guard<std::mutex> guard(mutex_query09_subquery01);
        // Set the parameters
        (*parameters_query09_)[0] = percentage ? utils_remove_percentage(parameter_vector[0]) : parameter_vector[0];
        (*parameters_query09_)[1] = parameter_vector[1];
        (*parameters_query09_)[2] = parameter_vector[2];
        (*parameters_query09_)[3] = parameter_vector[3];
        t_f = parameter_vector[4];

        // Create the QueryCondition for Subquery01
        dds_entity_t queryCondition_query09_subquery01 =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(filter_query09_subquery01));

        // Perform Subquery01
        samples_received_subquery01 = dds_read(queryCondition_query09_subquery01, msgs_pointers_subquery01, msgs_infos_subquery01, maximum_messages_, maximum_messages_);
        dds_delete(queryCondition_query09_subquery01);

        // Get the maximum id only
        int current_max = 0;
        for (int i = 0; i < samples_received_subquery01; ++i)
        {
            if (msgs_infos_subquery01[i].valid_data)
            {
                if (msgs_data_subquery01[i].id() > current_max)
                {
                    // FIXME: Only theid is needed, don't need to keep the entire message
                    current_max = msgs_data_subquery01[i].id();
                    QueryStructure::structure_generic tmp(max_id, compaction_prev_revision,
                                                          msgs_data_subquery01[i].id(), msgs_data_subquery01[i].name(),
                                                          msgs_data_subquery01[i].created(), msgs_data_subquery01[i].deleted(),
                                                          msgs_data_subquery01[i].create_revision(), msgs_data_subquery01[i].prev_revision(),
                                                          msgs_data_subquery01[i].lease(),
                                                          msgs_data_subquery01[i].value(), msgs_data_subquery01[i].old_value());
                    result_subquery01 = tmp;
                }
            }
        }

        // Prevent race conditions with the parameter vector

        // std::lock_guard<std::mutex> guard(mutex_query09_subquery02);
        // Now set the parameter for the filter to the result of Subquery01
        (*parameters_query09_)[5] = std::to_string(result_subquery01.theid);

        // Create the QueryCondition for Subquery02
        dds_entity_t queryCondition_query09_subquery02 =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(percentage ? filter_query09_subquery02_percentage : filter_query09_subquery02));

        // Perform the Subquery02
        samples_received_subquery02 = dds_read(queryCondition_query09_subquery02, msgs_pointers_subquery02, msgs_infos_subquery02, msgs_number, msgs_number);
        dds_delete(queryCondition_query09_subquery02);
    }

    // Perform the GROUP BY, the JOIN and ORDER BY id ASC. No need to do that if no messages were read.
    if (samples_received_subquery02 > 0)
    {
        result = query_common_subquery(samples_received_subquery02, &msgs_infos_subquery02[0],
                                       &msgs_data_subquery02[0], max_id, compaction_prev_revision, t_f);
        // Apply the limit to ensure the same order and result as in SQL
        if (limit > 0 and result.size() > limit)
            result.resize(limit);
    }

    return result;
}

std::vector<QueryStructure::structure_generic> Database::query_11(int limit, std::vector<std::string> parameter_vector)
{
    // Create the result vectors
    std::vector<QueryStructure::structure_generic> result;
    // Create the result map
    std::map<std::string, std::pair<QueryStructure::structure_generic, bool>> result_subquery01; // The second bool indicates whether the result is valid
    int samples_received_subquery01 = 0;
    std::string t_f = "f";

    // Get the maximum id and the prev_revision of the compaction
    unsigned long max_id, compaction_prev_revision;
    query_find_common_values(&max_id, &compaction_prev_revision);

    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data_subquery01[msgs_number];
    void *msgs_pointers_subquery01[msgs_number];
    dds_sample_info_t msgs_infos_subquery01[msgs_number];

    // // Make the pointers point to the data for both subqueries
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers_subquery01[i] = &msgs_data_subquery01[i];
    }

    // Essentially, the process followed is:
    // 0. Prepare parameters and create the QueryCondition
    // 1. Perform subquery01
    // 3. Merge the results performing a join on the id while evaluating results from Subquery01

    bool percentage = utils_has_percentage(parameter_vector[0]);

    {
        std::lock_guard<std::mutex> guard(mutex_query11);
        // Prepare parameters for Subquery01 (element [0]) and for Subquery02 (element [1])
        (*parameters_query11_)[0] = percentage ? utils_remove_percentage(parameter_vector[0]) : parameter_vector[0];
        (*parameters_query11_)[1] = parameter_vector[1];
        t_f = parameter_vector[2];

        // Create the QueryConditions
        dds_entity_t queryCondition_query11_subquery01 =
            dds_create_querycondition((*reader_handle_), DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(percentage ? filter_query11_subquery01_percentage : filter_query11_subquery01));

        // Perform subquery01
        samples_received_subquery01 = dds_read(queryCondition_query11_subquery01, msgs_pointers_subquery01, msgs_infos_subquery01, maximum_messages_, maximum_messages_);
        dds_delete(queryCondition_query11_subquery01);
    }

    // Perform the GROUP BY, the JOIN and ORDER BY id ASC. No need to do that if no messages were read.
    if (samples_received_subquery01 > 0)
    {
        result = query_common_subquery(samples_received_subquery01, &msgs_infos_subquery01[0], &msgs_data_subquery01[0],
                                       max_id, compaction_prev_revision, t_f);

        // Apply the limit to ensure the same order and result as in SQL
        if (limit > 0 and result.size() > limit)
            result.resize(limit);
    }

    return result;
}

// +------------------------+
// | QUERYCONDITION FILTERS |
// +------------------------+
bool Database::filter_mass_dispose_compaction_subquery01(const DatabaseData::Msg *message)
{
    return message->name() != "compact_rev_key" and
           message->prev_revision() != 0 and
           message->id() <= (*parameters_mass_dispose_compaction_)[0];
}

bool Database::filter_mass_dispose_compaction_subquery02(const DatabaseData::Msg *message)
{
    return message->deleted() != 0 and
           message->id() <= (*parameters_mass_dispose_compaction_)[1] and
           message->id() == (*parameters_mass_dispose_compaction_)[2];
}

bool Database::filter_query_compaction(const DatabaseData::Msg *message)
{
    return message->name() == "compact_rev_key";
}

bool Database::filter_query_find_message_by_id(const DatabaseData::Msg *message)
{
    // WHERE id = $1
    return message->id() == parameters_id_;
}

bool Database::filter_query01(const DatabaseData::Msg *message)
{
    // WHERE name LIKE $1 AND id < $2
    return message->name() == (*parameters_query01_)[0] and
           message->id() > std::stoi((*parameters_query01_)[1]);
}

bool Database::filter_query01_percentage(const DatabaseData::Msg *message)
{
    // WHERE name LIKE $1 AND id < $2
    return message->name().find((*parameters_query01_)[0]) != std::string::npos and
           message->id() > std::stoi((*parameters_query01_)[1]);
}

bool Database::filter_query04_subquery01(const DatabaseData::Msg *message)
{
    // WHERE name LIKE $1
    return message->name() == (*parameters_query04_)[0];
}

bool Database::filter_query04_subquery01_percentage(const DatabaseData::Msg *message)
{
    // WHERE name LIKE $1
    return message->name().find((*parameters_query04_)[0]) != std::string::npos;
}

bool Database::filter_query09_subquery01(const DatabaseData::Msg *message)
{
    return message->name() == (*parameters_query09_)[2] and
           message->id() <= std::stoi((*parameters_query09_)[3]);
}

bool Database::filter_query09_subquery02(const DatabaseData::Msg *message)
{
    return message->name() == (*parameters_query09_)[0] and
           message->id() <= std::stoi((*parameters_query09_)[1]) and
           message->id() > std::stoi((*parameters_query09_)[5]);
}

bool Database::filter_query09_subquery02_percentage(const DatabaseData::Msg *message)
{
    return message->name().find((*parameters_query09_)[0]) != std::string::npos and
           message->id() <= std::stoi((*parameters_query09_)[1]) and
           message->id() > std::stoi((*parameters_query09_)[5]);
}

bool Database::filter_query11_subquery01(const DatabaseData::Msg *message)
{
    // SELECT MAX(mkv.id) AS id
    //             FROM kine AS mkv
    //             WHERE mkv.name LIKE $1
    //                 AND mkv.id <= $2
    //             GROUP BY mkv.name
    return message->name() == (*parameters_query11_)[0] and
           message->id() <= std::stoi((*parameters_query11_)[1]);
}

bool Database::filter_query11_subquery01_percentage(const DatabaseData::Msg *message)
{
    // SELECT MAX(mkv.id) AS id
    //             FROM kine AS mkv
    //             WHERE mkv.name LIKE $1
    //                 AND mkv.id <= $2
    //             GROUP BY mkv.name
    return message->name().find((*parameters_query11_)[0]) != std::string::npos and
           message->id() <= std::stoi((*parameters_query11_)[1]);
}

// +-----------------------------------+
// | FUNCTIONS THAT EXPOSE THE SERVICE |
// +-----------------------------------+
std::pair<unsigned long, bool> Database::expose_publish(int id, std::string name,
                                                        unsigned long created, unsigned long deleted,
                                                        unsigned long create_revision, unsigned long prev_revision,
                                                        unsigned long lease,
                                                        std::vector<unsigned char> value, std::vector<unsigned char> old_value)
{
    // If id is < 0, needs to return the returning id
    if (id < 0)
    {
        return core_publish_returningid(name,
                                        created, deleted,
                                        create_revision, prev_revision,
                                        lease,
                                        value, old_value);
    }
    else
    {
        // Used mainly by updates (SetCompactRevision, Fill)
        return core_publish(id, name,
                            created, deleted,
                            create_revision, prev_revision,
                            lease,
                            value, old_value);
    }
}

bool Database::expose_dispose(unsigned long id)
{
    DatabaseData::Msg message = query_find_message_by_id(id);
    if (message.name() == "empty")
    {
        cout << "== MESSAGE WITH id " << id << " does not exist!" << endl;
        return false;
    }
    return core_dispose(query_find_message_by_id(id));
}

QueryStructure::structure_massDisposeCompaction Database::expose_massDisposeCompaction(int param1, int param2)
{
    return query_mass_dispose_compaction(param1, param2);
}

void Database::TESTING_update_message()
{
    int id, created, deleted, create_revision, prev_revision;
    std::string name;
    cout << "Id of the message to modify:" << endl;
    cin >> id;
    cout << "New value for the name: " << endl;
    cin >> name;

    DatabaseData::Msg tmp = query_find_message_by_id(id);

    core_publish(id, name, tmp.created(), tmp.deleted(), tmp.create_revision(), tmp.prev_revision(), tmp.lease(), tmp.value(), tmp.old_value());

    TESTING_print_all_history();
}

std::vector<QueryStructure::structure_generic> Database::expose_querygeneric(const int query_identifier, int limit,
                                                                             std::string param1, std::string param2,
                                                                             std::string param3, std::string param4, std::string param5)
{
    // Create common vectors
    std::vector<std::string> params;
    params.push_back(param1);
    params.push_back(param2);

    std::vector<QueryStructure::structure_generic> result;

    switch (query_identifier)
    {
    case 1:
        // 2 parameters (already pushed back)
        // Launch query
        result = query_01(limit, params);
        break;
    case 4:
        // 2 parameters (already pushed back)
        // Launch query
        result = query_04(limit, params);
        break;
    case 9:
        // 5 parameters (3 to be added)
        params.push_back(param3);
        params.push_back(param4);
        params.push_back(param5);

        // Launch query
        result = query_09(limit, params);
        break;
    case 11:
        // 3 parameters (1 to be added)
        params.push_back(param3);

        // Launch query
        result = query_11(limit, params);
        break;
    default:
        cout << "== ERROR: THE QUERY NUMBER IS NOT A VALID QUERY" << endl;
        break;
    }

    return result;
}

QueryStructure::structure_query03 Database::expose_query03(std::string param1, std::string param2)
{
    std::vector<std::string> params;
    params.push_back(param1);
    params.push_back(param2);
    return query_03(params);
}

unsigned long Database::expose_querycompactrevision()
{
    return query_compaction().id();
}

std::pair<unsigned long, bool> Database::expose_querymaximumid()
{
    return query_maxid();
}

// +-----------------------+
// | MEASUREMENT FUNCTIONS |
// +-----------------------+
//! MEASUREMENT CODE
// void Database::print_measurements()
// {
    // std::lock_guard<std::mutex> guard(mutex_measurement_vector);
    // for (auto measurement : measurement_vector)
    // {
        // cout << measurement << endl;
    // }
// }

void Database::print_message_sizes() {
    cout << "== Printing all messages " << endl;
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    int samples_received = dds_read((*reader_handle_), msgs_pointers, msgs_infos, msgs_number, msgs_number);

    cout << "message_id,size_longs,size_name,size_values,size_oldvalue,size_total" << endl;
    for (int i = 0; i < samples_received; i++)
    {
        int size_longs = 4 * 6; // id, created, deleted, cr, pr, lease. 4B each
        int size_name = msgs_data[i].name().size();
        int size_value = msgs_data[i].value().size(); // 1B per char
        int size_oldvalue =  msgs_data[i].old_value().size();
        int size_total = size_longs + size_name + size_value + size_oldvalue;
        
        cout << msgs_data[i].id() << "," << size_longs << "," << size_name << "," << size_value << "," << size_oldvalue << "," << size_total << endl;         
    }
    cout << "=== Done printing valid messages ===" << endl;
}

// +-------------------+
// | TESTING FUNCTIONS |
// +-------------------+
void Database::TESTING_print_vector(std::vector<uint8_t> v)
{
    for (auto i : v)
    {
        std::cout << i;
    }
}

void Database::TESTING_print_message(DatabaseData::Msg msg)
{
    std::cout << "Id: " << msg.id()
              << " | Name: " << msg.name()
              << " | Created: " << msg.created()
              << " | Deleted: " << msg.deleted()
              << " | Create_revision: " << msg.create_revision()
              << " | Prev_revision: " << msg.prev_revision()
              << " | Lease: " << msg.lease()
              << " | Value: ";

    // Print the values vectors
    // cout << endl;
    // TESTING_print_vector(msg.value());
    // std::cout << endl
    //           << " | Old_value: " << endl;
    // TESTING_print_vector(msg.old_value());
    // cout << endl;

    std::cout << std::endl;
}

void Database::TESTING_print_structure_generic(QueryStructure::structure_generic to_print)
{
    std::cout << "maxid: " << to_print.max_id << "  |  "
              << "cpr: " << to_print.compaction_prev_revision << "  |  "
              << "theid: " << to_print.theid << "  |  "
              << "name: " << to_print.name << "  |  "
              << "created: " << to_print.created << "  |  "
              << "deleted: " << to_print.deleted << "  |  "
              << "cr: " << to_print.create_revision << "  |  "
              << "pr: " << to_print.prev_revision << "  |  "
              << "lease: " << to_print.lease
              << std::endl;
}

void Database::TESTING_print_structure_query03(QueryStructure::structure_query03 to_print)
{
    std::cout << "maxid: " << to_print.id << "  |  "
              << "count: " << to_print.count
              << std::endl;
}

void Database::TESTING_publish()
{
    bool returning = false;
    unsigned long id, created, deleted, create_revision, prev_revision, lease;
    std::string name, svalue, sold_value, response;

    std::cout << "Do you want to assign the id automatically? (y, n)" << endl;
    cin >> response;

    if (response == "y")
        returning = true;

    if (!returning)
    {
        std::cout << "id: ";
        std::cin >> id;
    }
    else
        id = 0;
    std::cout << "name: ";
    std::cin >> name;
    std::cout << "created: ";
    std::cin >> created;
    std::cout << "deleted: ";
    std::cin >> deleted;
    std::cout << "create_revision: ";
    std::cin >> create_revision;
    std::cout << "prev_revision: ";
    std::cin >> prev_revision;
    std::cout << "lease: ";
    std::cin >> lease;
    std::cout << "value: ";
    std::cin >> svalue;
    std::cout << "old_value: ";
    std::cin >> sold_value;

    std::vector<unsigned char> value(svalue.begin(), svalue.end());
    std::vector<unsigned char> old_value(sold_value.begin(), sold_value.end());

    if (!returning)
        core_publish(id, name, created, deleted, create_revision, prev_revision, lease, value, old_value);
    else
        cout << "The id assigned to the new message is: "
             << core_publish_returningid(name, created, deleted, create_revision, prev_revision, lease, value, old_value).first
             << endl;
}

void Database::TESTING_dispose()
{
    dds::sub::DataReader<DatabaseData::Msg> rd = *reader_;
    dds_entity_t rdhandle = (*reader_)->get_ddsc_entity();
    dds_entity_t qc = dds_create_querycondition(rdhandle, DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(TESTING_filter_test));

    int MAXSAMPLES = 20;
    DatabaseData::Msg data[MAXSAMPLES];
    void *samples[MAXSAMPLES];
    dds_sample_info_t infos[MAXSAMPLES];

    memset(data, 0, sizeof(data));
    for (int i = 0; i < MAXSAMPLES; ++i)
    {
        samples[i] = &data[i];
    }

    int samples_received = dds_read(qc, samples, infos, MAXSAMPLES, MAXSAMPLES);
    for (int i = 0; i < samples_received; ++i)
    {
        TESTING_print_message(data[i]);
        core_dispose(data[0]);
    }
}

void Database::TESTING_count()
{
    std::cout << "Counted: " << query_count_all() << std::endl;
}

void Database::TESTING_print_all_history()
{
    cout << "== Printing all messages " << endl;
    // Only take into account messages that are alive
    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    int samples_received = dds_read((*reader_handle_), msgs_pointers, msgs_infos, msgs_number, msgs_number);

    // cout << "=== Printing all messages ===" << endl;
    // for (int i = 0; i < samples_received; i++)
    // {
    //     TESTING_print_message(msgs_data[i]);
    // }
    // cout << "=== Done printing all messages ===" << endl;
    cout << "\n=== Printing valid messages ===" << endl;
    for (int i = 0; i < samples_received; i++)
    {
        if (msgs_infos[i].valid_data)
            TESTING_print_message(msgs_data[i]);
    }
    cout << "=== Done printing valid messages ===" << endl;
    // cout << "\n=== Printing messages without writers ===" << endl;
    // for (int i = 0; i < samples_received; i++)
    // {
    //     if (msgs_infos[i].instance_state == DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
    //         TESTING_print_message(msgs_data[i]);
    // }
    // cout << "=== Done printing messages without writers ===" << endl;
}

void Database::TESTING_print_all_history_maxid()
{
    cout << "== Printing all messages " << endl;
    // Only take into account messages that are alive
    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    IdentifierTracker::MaxIdMsg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    int samples_received = dds_read((*reader_maxid_handle_), msgs_pointers, msgs_infos, msgs_number, msgs_number);

    // cout << "=== Printing all messages ===" << endl;
    // for (int i = 0; i < samples_received; i++)
    // {
    //     TESTING_print_message(msgs_data[i]);
    // }
    // cout << "=== Done printing all messages ===" << endl;
    cout << "\n=== Printing valid messages ===" << endl;
    for (int i = 0; i < samples_received; i++)
    {
        if (msgs_infos[i].valid_data)
            cout << "MaxId: " << msgs_data[i].maxid() << endl;
    }
    cout << "=== Done printing valid messages ===" << endl;
    // cout << "\n=== Printing messages without writers ===" << endl;
    // for (int i = 0; i < samples_received; i++)
    // {
    //     if (msgs_infos[i].instance_state == DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
    //         cout << "MaxId: " << msgs_data[i].maxid() << endl;
    // }
    // cout << "=== Done printing messages without writers ===" << endl;
}

bool Database::TESTING_filter_test(const DatabaseData::Msg *sample)
{
    return sample->name().find("bootstrap") != std::string::npos;
}

void Database::TESTING_query()
{
    std::string query_option;
    cout << "== Select the query to test" << endl;
    std::cin >> query_option;
    if (query_option == "test")
    {
        // Get the handle of the reader
        dds_entity_t rdhandle = (*reader_)->get_ddsc_entity();
        // Create the querycondition, bounding it with the reader and with the filter function
        dds_entity_t qc = dds_create_querycondition(rdhandle, DDS_ALIVE_INSTANCE_STATE, dds_querycondition_filter_fn(TESTING_filter_test));

        // Create the return structures
        int MAXSAMPLES = 20;
        DatabaseData::Msg data[MAXSAMPLES];
        void *samples[MAXSAMPLES];
        dds_sample_info_t infos[MAXSAMPLES];

        // Get the pointers to point to the data
        for (int i = 0; i < MAXSAMPLES; ++i)
        {
            samples[i] = &data[i];
        }

        int samples_received = dds_read(qc, samples, infos, MAXSAMPLES, MAXSAMPLES);
        for (int i = 0; i < samples_received; ++i)
        {
            TESTING_print_message(data[i]);
        }

        cout << "== Done testing query test" << endl;
    }
    else if (query_option == "query01")
    {
        std::string s1, s2;
        int lim;
        cout << "Query uses: name LIKE $1 AND id < $2." << endl
             << "Input $1: " << endl;
        cin >> s1;
        cout << "Input $2: " << endl;
        cin >> s2;
        cout << "Input limit of results: " << endl;
        cin >> lim;
        std::vector<std::string> params;
        params.push_back(s1);
        params.push_back(s2);
        std::vector<QueryStructure::structure_generic> result = query_01(lim, params);
        for (auto r : result)
        {
            TESTING_print_structure_generic(r);
        }
        cout << "== Done testing query01" << endl;
    }
    else if (query_option == "query03")
    {
        std::string s1, s2;
        cout << "Input $1 (ex. /registry/pods): " << endl;
        cin >> s1;
        cout << "Input $2 ('f' or 't'): " << endl;
        cin >> s2;
        std::vector<std::string> params;
        params.push_back(s1);
        params.push_back(s2);
        auto result = query_03(params);
        TESTING_print_structure_query03(result);
        cout << "== Done testing query03" << endl;
    }
    else if (query_option == "query04")
    {
        std::string s1, s2;
        int lim;
        cout << "Input $1 (ex. /registry/pods): " << endl;
        cin >> s1;
        cout << "Input $2 ('f' or 't'): " << endl;
        cin >> s2;
        cout << "Input limit of results: " << endl;
        cin >> lim;
        std::vector<std::string> params;
        params.push_back(s1);
        params.push_back(s2);
        std::vector<QueryStructure::structure_generic> result = query_04(lim, params);
        for (auto e : result)
            TESTING_print_structure_generic(e);
        cout << "== Done testing query04" << endl;
    }
    else if (query_option == "query09")
    {
        std::string s1, s2, s3, s4, s5;
        int lim;
        cout << "Input $1 (ex. /registry/events/default/): " << endl;
        cin >> s1;
        cout << "Input $2 (ex. 10000): " << endl;
        cin >> s2;
        cout << "Input $3: (ex. '/registry/events/default/masteroo.176c78372d61811f')" << endl;
        cin >> s3;
        cout << "Input $4 (ex. 10000): " << endl;
        cin >> s4;
        cout << "Input $5 ('f' or 't'): " << endl;
        cin >> s5;
        cout << "Input limit of results: " << endl;
        cin >> lim;
        std::vector<std::string> params;
        params.push_back(s1);
        params.push_back(s2);
        params.push_back(s3);
        params.push_back(s4);
        params.push_back(s5);
        std::vector<QueryStructure::structure_generic> result = query_09(lim, params);
        for (auto e : result)
            TESTING_print_structure_generic(e);
        cout << "== Done testing query09" << endl;
    }
    else if (query_option == "query11")
    {
        std::string s1, s2, s3;
        int lim;
        cout << "Input $1: " << endl;
        cin >> s1;
        cout << "Input $2: " << endl;
        cin >> s2;
        cout << "Input $3: " << endl;
        cin >> s3;
        cout << "Input limit of results: " << endl;
        cin >> lim;
        std::vector<std::string> params;
        params.push_back(s1);
        params.push_back(s2);
        params.push_back(s3);
        std::vector<QueryStructure::structure_generic> result = query_11(lim, params);
        for (auto e : result)
            TESTING_print_structure_generic(e);
        cout << "== Done testing query11" << endl;
    }
    else
    {
        cout << "== Invalid query :(" << endl;
    }
}

void Database::TESTING_print_max()
{
    std::pair<unsigned long, bool> max = query_maxid();
    cout << "Maximum id is: " << max.first << " . Is the result ok? " << max.second << endl;
}

void Database::TESTING_print_disposed()
{
    // Only take into account messages that are alive
    dds::sub::LoanedSamples<DatabaseData::Msg> msgs = reader_->select().state(dds::sub::status::InstanceState::not_alive_disposed()).read();
    // Iterate through all the alive messages to find the one with the maximum id
    if (msgs.length() > 0)
    {
        dds::sub::LoanedSamples<DatabaseData::Msg>::const_iterator i;
        for (i = msgs.begin(); i < msgs.end(); ++i)
        {
            if (i->info().valid())
            {
                TESTING_print_message(i->data());
            }
        }
    }
    else
        cout << "== No messages found!" << endl;
}

void Database::TESTING_populate(int count)
{
    std::string sv1 = "Valueroo";
    std::string sv2 = "Old valueroo";
    std::vector<uint8_t> v1(sv1.begin(), sv1.end());
    std::vector<uint8_t> v2(sv2.begin(), sv2.end());

    int i = 0;
    char line[2000];
    std::ifstream read_file;
    read_file.open("/root/workspace/CycloneDatabase/20231109_KineContent.csv");

    while (read_file.getline(line, 20000) and i < count)
    {
        std::string id, name, created, deleted, create_revision, prev_revision, lease, value, old_value = "";
        int field_num = 0;

        for (char c : line)
        {
            if (c == ',')
            {
                ++field_num;
            }
            else if (field_num >= 9)
            {
                std::vector<uint8_t> v1(value.begin(), value.end());
                std::vector<uint8_t> v2(old_value.begin(), old_value.end());
                if (i != 0)
                {
                    core_publish(std::stoull(id), name, std::stoull(created), std::stoull(deleted),
                                 std::stoull(create_revision), std::stoull(prev_revision),
                                 std::stoull(lease), v1, v2);
                }
                ++i;
                field_num = 0;
                id, name, created, deleted, create_revision, prev_revision, lease, value, old_value = "";
                break;
            }
            else
            {
                switch (field_num)
                {
                case 0:
                    id += c;
                    break;
                case 1:
                    name += c;
                    break;
                case 2:
                    created += c;
                    break;
                case 3:
                    deleted += c;
                    break;
                case 4:
                    create_revision += c;
                    break;
                case 5:
                    prev_revision += c;
                    break;
                case 6:
                    lease += c;
                    break;
                case 7:
                    value += c;
                    break;
                case 8:
                    old_value += c;
                    break;
                default:
                    std::cout << "Whoops, something went wrong" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    break;
                }
            }
        }
    }
}

void Database::TESTING_get_options()
{
    std::string option;
    std::cin >> option;

    if (option == "quit")
        Database::done_ = true;
    else if (option == "publish")
        TESTING_publish();
    else if (option == "dispose")
        TESTING_dispose();
    else if (option == "printall")
        TESTING_print_all_history();
    else if (option == "query")
        TESTING_query();
    else if (option == "printdisposed")
        TESTING_print_disposed();
    else if (option == "printmax")
        TESTING_print_max();
    else if (option == "count")
        TESTING_count();
    else if (option == "populate")
    {
        cout << "How many samples?" << endl;
        int i;
        std::cin >> i;
        TESTING_populate(i);
    }
    else if (option == "update")
    {
        TESTING_update_message();
    }
    else if (option == "findid")
    {
        unsigned long id;
        cout << "Input id" << endl;
        cin >> id;
        DatabaseData::Msg tmp = query_find_message_by_id(id);
        TESTING_print_message(tmp);
    }
    else if (option == "printcompaction")
        TESTING_print_message(query_compaction());
    else
        std::cout << "Invalid option :(" << std::endl;
}

// +----------------------+
// | WORKAROUND FUNCTIONS |
// +----------------------+
void Database::republish_message(DatabaseData::Msg msg)
{
    auto res = republished_messages.find(msg.id());
    if (res != republished_messages.end())
    {
        if (matched > 0)
        {
            try
            {
                writer_->write(msg);
                republished_messages.emplace(msg.id());
            }
            catch (const dds::core::Exception &e)
            {
                std::cout << "# Error in republish: \"" << e.what() << std::endl;
            }
        }
        else
            cout << "==== No reader is available in republish" << endl;
    }
}

void Database::republish_message_maxid()
{
    IdentifierTracker::MaxIdMsg msg;
    msg.maxid() = query_maxid().first;

    if (matched > 0)
    {
        try
        {
            writer_maxid_->write(msg);
        }
        catch (const dds::core::Exception &e)
        {
            std::cout << "# Error in republish: \"" << e.what() << std::endl;
        }
    }
    else
        cout << "==== No reader is available in republish" << endl;
}

void Database::republish_everything()
{
    // Only take into account messages that are alive
    // Create structures to store the messages
    int msgs_number = maximum_messages_;
    DatabaseData::Msg msgs_data[msgs_number];
    void *msgs_pointers[msgs_number];
    dds_sample_info_t msgs_infos[msgs_number];

    // Make the pointers point to the data
    for (int i = 0; i < msgs_number; ++i)
    {
        msgs_pointers[i] = &msgs_data[i];
    }

    int samples_received = dds_read((*reader_handle_), msgs_pointers, msgs_infos, msgs_number, msgs_number);

    for (int i = 0; i < samples_received; i++)
    {
        if (msgs_infos[i].instance_state == DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
        {
            if (msgs_data[i].name() != "" and matched > 0)
            {
                try
                {
                    writer_->write(msgs_data[i]);
                }
                catch (const dds::core::Exception &e)
                {
                    std::cout << "# Error in republish: \"" << e.what() << std::endl;
                }
            }
        }
    }
    republish_message_maxid();
    // Sleep a bit to allow for all empty messages to be received
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    started_republish.store(false);
}
