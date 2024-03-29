/**
 * @file Controller.hpp
 * @author Saül Abad Copoví
 * @brief Controller header
 */

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <grpcpp/grpcpp.h>

#include "Communication.grpc.pb.h"
#include "QueryStructure.h"
#include "Database.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using MessageStructures::DatabaseCommunication;
using MessageStructures::KineDispose;
using MessageStructures::KineMassDisposeCompaction;
using MessageStructures::KineInsert;
using MessageStructures::KineQueryCompactRevision;
using MessageStructures::KineQueryGeneric;
using MessageStructures::KineQueryMaximumId;
using MessageStructures::KineQueryThree;
using MessageStructures::KineTest;
using MessageStructures::ResponseDispose;
using MessageStructures::ResponseMassDisposeCompaction;
using MessageStructures::ResponsePublish;
using MessageStructures::ResponseQueryCompactRevision;
using MessageStructures::ResponseQueryGeneric;
using MessageStructures::ResponseQueryMaximumId;
using MessageStructures::ResponseQueryThree;
using MessageStructures::ResponseTest;

class Controller
{
private:
    std::shared_ptr<Database> db_;

public:
    Controller();
    virtual ~Controller();

    void initialise_controller();
    void run_controller();

    std::pair<unsigned long, bool> publish(int64_t id, std::string name,
                                           unsigned long created, unsigned long deleted,
                                           unsigned long create_revision, unsigned long prev_revision,
                                           unsigned long lease,
                                           std::string value, std::string old_value);

    std::vector<QueryStructure::structure_generic> query_generic(int query_identifier, int limit,
                                                                 std::string param1, std::string param2, std::string param3, std::string param4, std::string param5);
    QueryStructure::structure_query03 query_three(std::string param1, std::string param2);

    unsigned long query_compactrevision();

    unsigned long query_maximumid();

    bool dispose(int id);

    QueryStructure::structure_massDisposeCompaction massDisposeCompaction(int param1, int param2);
};



// ----------------



// This code for gRPC is included here because of having followed the example given on the gRPC at the moment of implementing the communication
// Would be better put into a separate file. Regardless, it has been kept here

class CommunicationServiceImplementation final : public DatabaseCommunication::Service
{
    Status RequestPublish(ServerContext *context, const KineInsert *request, ResponsePublish *reply) override
    {
        // std::cout << ">>>> REQUESTED PUBLISH" << std::endl;
        std::pair<unsigned long, bool> result = controller_->publish(request->id(), request->name(),
                                                                     request->created(), request->deleted(),
                                                                     request->create_revision(), request->prev_revision(),
                                                                     request->lease(),
                                                                     request->value(), request->old_value());
        reply->set_returning_id(result.first);
        reply->set_success(result.second);
        return Status::OK;
    }

    Status RequestDispose(ServerContext *context, const KineDispose *request, ResponseDispose *reply) override
    {
        // std::cout << ">>>> REQUESTED DISPOSE" << std::endl;
        bool result = controller_->dispose(request->id());
        reply->set_success(result);
        return Status::OK;
    }

    Status RequestMassDisposeCompaction(ServerContext *context, const KineMassDisposeCompaction *request, ResponseMassDisposeCompaction *reply) override {
        QueryStructure::structure_massDisposeCompaction result = controller_->massDisposeCompaction(request->param1(), request->param2());
        reply->set_success(result.success);
        reply->set_deleted_rows(result.rows_disposed);
        return Status::OK;
    }

    Status RequestQueryGeneric(ServerContext *context, const KineQueryGeneric *request, ResponseQueryGeneric *reply) override
    {
        // std::cout << "LIMIT IS " << request->limit() << " AND QUERY IS " << request->query_identifier() << std::endl;
        // std::cout << ">>>> REQUESTED QUERY GENERIC" << std::endl;
        auto result = controller_->query_generic(request->query_identifier(), request->limit(),
                                                 request->param1(), request->param2(), request->param3(), request->param4(), request->param5());
        for (int i = 0; i < result.size(); ++i)
        {
            // Add a new row in the reply message
            MessageStructures::QueryGenericRow *r = reply->add_row();

            // Fill the fields of the row added
            r->set_max_id(result[i].max_id);
            r->set_compaction_prev_revision(result[i].compaction_prev_revision);
            r->set_theid(result[i].theid);
            r->set_name(result[i].name);
            r->set_created(result[i].created);
            r->set_deleted(result[i].deleted);
            r->set_create_revision(result[i].create_revision);
            r->set_prev_revision(result[i].prev_revision);
            r->set_lease(result[i].lease);

            // Cast the values into a string (C++ type for a Protobuf type "bytes") and add them to the message
            std::string svalue(result[i].value.begin(), result[i].value.end());
            std::string sold_value(result[i].old_value.begin(), result[i].old_value.end());

            r->set_value(svalue);
            r->set_old_value(sold_value);
        }
        return Status::OK;
    }

    Status RequestQueryThree(ServerContext *context, const KineQueryThree *request, ResponseQueryThree *reply) override
    {
        // std::cout << ">>>> REQUESTED QUERY THREE" << std::endl;
        auto result = controller_->query_three(request->param1(), request->param2());
        reply->set_count(result.count);
        reply->set_id(result.id);
        return Status::OK;
    }

    Status RequestQueryCompactRevision(ServerContext *context, const KineQueryCompactRevision *request, ResponseQueryCompactRevision *reply) override
    {
        // std::cout << ">>>> REQUESTED QUERY COMPACT REVISION" << std::endl;
        auto result = controller_->query_compactrevision();
        reply->set_id(result);
        return Status::OK;
    }

    Status RequestQueryMaximumId(ServerContext *context, const KineQueryMaximumId *request, ResponseQueryMaximumId *reply) override
    {
        // std::cout << ">>>> REQUESTED QUERY MAXIMUM ID" << std::endl;
        auto result = controller_->query_maximumid();
        reply->set_id(result);
        return Status::OK;
    }

    Status RequestTest(ServerContext *context, const KineTest *request, ResponseTest *reply) override
    {
        std::cout << ">>>> REQUESTED TEST" << std::endl;
        std::cout << ">>> Id of the test message is: " << request->testid() << " and content is \"" << request->content() << "\"" << std::endl;
        reply->set_responseid(request->testid());
        std::string s = "I am returning a message with the same id I received it with";
        reply->set_content(s);
        return Status::OK;
    }

public:
    void
    set_controller(Controller *controller)
    {
        controller_ = controller;
    }

private:
    Controller *controller_;
};