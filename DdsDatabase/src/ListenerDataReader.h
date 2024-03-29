/**
 * @file ListenerDataReader.h
 * @author your name (you@domain.com)
 * @brief 
 */
#ifndef LISTENER_DATAREADER_H
#define LISTENER_DATAREADER_H

#include <functional>
#include "dds/dds.hpp"
#include "DatabaseData.hpp"


class ListenerDataReader : public dds::sub::NoOpDataReaderListener<DatabaseData::Msg>
{
public:
    using callback_func_dataavailable = std::function<bool(dds::sub::DataReader<DatabaseData::Msg>&)>;
    using callback_func_subscriptionmatched = std::function<bool(dds::sub::DataReader<DatabaseData::Msg>&)>;

    /**
     * @brief Empty constructor, avoids creating an object without parameters
     * 
     */
    ListenerDataReader() = delete;

    /**
     * @brief Construct a new Listener Data Reader object
     * 
     * @param function_dataavailable, function implemented in Database that handles the receival of samples 
     * @param function_publicationmatched, function implemented in Database that handles matching with publishers 
     */
    ListenerDataReader(const callback_func_dataavailable &function_dataavailable, const callback_func_subscriptionmatched &function_subscriptionmatched)
        : dds::sub::DataReaderListener<DatabaseData::Msg>(),
          function_dataavailable_(function_dataavailable),
          function_subscriptionmatched_(function_subscriptionmatched)
    { ; }

    /**
     * @brief Overrides the default on_data_available call
     * 
     */
    void on_data_available(dds::sub::DataReader<DatabaseData::Msg>& rd)
    {
        (void)function_dataavailable_(rd);
    }

    void on_subscription_matched(dds::sub::DataReader<DatabaseData::Msg>& rd) {
        (void)function_subscriptionmatched_(rd);
    }



private:
    /**
     * @brief Reference to the function that is called when the listener is activated
     * 
     */
    callback_func_dataavailable function_dataavailable_;
    callback_func_subscriptionmatched function_subscriptionmatched_;
};

#endif // LISTENER_DATAREADER_H



#ifndef LISTENER_DATAREADERMAXID_H
#define LISTENER_DATAREADERMAXID_H
class ListenerDataReaderMaxId : public dds::sub::NoOpDataReaderListener<IdentifierTracker::MaxIdMsg>
{
public:
    using callback_func_dataavailable = std::function<bool(dds::sub::DataReader<IdentifierTracker::MaxIdMsg>&)>;
    using callback_func_subscriptionmatched = std::function<bool(dds::sub::DataReader<IdentifierTracker::MaxIdMsg>&)>;

    /**
     * @brief Empty constructor, avoids creating an object without parameters
     * 
     */
    ListenerDataReaderMaxId() = delete;

    /**
     * @brief Construct a new Listener Data Reader object
     * 
     * @param function_dataavailable, function implemented in Database that handles the receival of samples 
     * @param function_publicationmatched, function implemented in Database that handles matching with publishers 
     */
    ListenerDataReaderMaxId(const callback_func_dataavailable &function_dataavailable, const callback_func_subscriptionmatched &callback_func_subscriptionmatched)
        : dds::sub::DataReaderListener<IdentifierTracker::MaxIdMsg>(),
          function_dataavailable_(function_dataavailable),
          function_subscriptionmatched_(callback_func_subscriptionmatched)
    { ; }

    /**
     * @brief Overrides the default on_data_available call
     * 
     */
    void on_data_available(dds::sub::DataReader<IdentifierTracker::MaxIdMsg>& rd)
    {
        (void)function_dataavailable_(rd);
    }

    void on_subscription_matched(dds::sub::DataReader<IdentifierTracker::MaxIdMsg>& rd) {
        (void)function_subscriptionmatched_(rd);
    }



private:
    /**
     * @brief Reference to the function that is called when the listener is activated
     * 
     */
    callback_func_dataavailable function_dataavailable_;
    callback_func_subscriptionmatched function_subscriptionmatched_;
};

#endif // LISTENER_DATAREADERMAXID_H