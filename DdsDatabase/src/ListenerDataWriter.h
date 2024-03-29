/**
 * @file ListenerDataWriter
 * @author your name (you@domain.com)
 * @brief
 *
 */
#ifndef LISTENER_DATAWRITER_H
#define LISTENER_DATAWRITER_H

#include <functional>
#include "dds/dds.hpp"
#include "DatabaseData.hpp"

class ListenerDataWriter : public dds::pub::NoOpDataWriterListener<DatabaseData::Msg>
{
public:
    using callback_func_publicationmatched = std::function<bool(dds::pub::DataWriter<DatabaseData::Msg> &)>;

    /**
     * @brief Empty constructor, avoids creating an object without parameters
     *
     */
    ListenerDataWriter() = delete;

    /**
     * @brief Construct a new Listener Data Reader object
     *
     * @param function_publicationmatched
     */
    ListenerDataWriter(const callback_func_publicationmatched &function_publicationmatched)
        : dds::pub::DataWriterListener<DatabaseData::Msg>(),
          function_publicationmatched_(function_publicationmatched)
    { ; }

    /**
     * @brief 
     * 
     * @param wr 
     */
    void on_publication_matched(dds::pub::DataWriter<DatabaseData::Msg> &wr)
    {
        (void) function_publicationmatched_(wr);
    }

private:
    /**
     * @brief Reference to the function that is called when the listener is activated
     *
     */
    callback_func_publicationmatched function_publicationmatched_;
};

#endif // LISTENER_DATAWRITER_H


#ifndef LISTENER_DATAWRITERMAXID_H
#define LISTENER_DATAWRITERMAXID_H
class ListenerDataWriterMaxId : public dds::pub::NoOpDataWriterListener<IdentifierTracker::MaxIdMsg>
{
public:
    using callback_func_publicationmatched = std::function<bool(dds::pub::DataWriter<IdentifierTracker::MaxIdMsg> &)>;

    /**
     * @brief Empty constructor, avoids creating an object without parameters
     *
     */
    ListenerDataWriterMaxId() = delete;

    /**
     * @brief Construct a new Listener Data Reader object
     *
     * @param function_publicationmatched
     */
    ListenerDataWriterMaxId(const callback_func_publicationmatched &function_publicationmatched)
        : dds::pub::DataWriterListener<IdentifierTracker::MaxIdMsg>(),
          function_publicationmatched_(function_publicationmatched)
    { ; }

    /**
     * @brief
     *
     * @param rd
     */
    void on_publication_matched(dds::pub::DataWriter<IdentifierTracker::MaxIdMsg> &wr)
    {
        (void) function_publicationmatched_(wr);
    }

private:
    /**
     * @brief Reference to the function that is called when the listener is activated
     *
     */
    callback_func_publicationmatched function_publicationmatched_;
};

#endif // LISTENER_DATAWRITERMAXID_H