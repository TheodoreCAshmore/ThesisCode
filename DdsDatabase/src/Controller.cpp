#include "Controller.hpp"
#include "Database.hpp"

std::shared_ptr<CommunicationServiceImplementation> service_;

/**
 * @brief Construct a new Controller:: Controller object
 *
 */
Controller::Controller()
    : db_(nullptr)
{
}

/**
 * @brief Destroy the Controller:: Controller object
 *
 */
Controller::~Controller()
{
}

void Controller::initialise_controller()
{
    // Create the database
    // Set up the connections?
    std::shared_ptr<Database> db = std::make_shared<Database>(10000);

    // Set the pointer
    db_ = db;

    // Create a thread to handle the communication
    std::thread test(&Database::init, db_);
    run_controller();
}

void Controller::run_controller()
{
    //! Start the server
    std::string server_address("0.0.0.0:50051");
    std::shared_ptr<CommunicationServiceImplementation> service = std::make_shared<CommunicationServiceImplementation>();
    service_ = service;
    service_->set_controller(this);

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    // Listens on server_address port and avoids requiring credentials
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Registration of the instance used to communicate with clients (use of the manual implementation of the methods for handling requests)
    builder.RegisterService(&(*service_));
    // // Creates and runs the server
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "==== Server has been initialised" << std::endl;

    // Initiate a blocking wait
    server->Wait();
    std::cout << "==== Server left the wait" << std::endl;
}

std::pair<unsigned long, bool> Controller::publish(int64_t id, std::string name,
                                                   unsigned long created, unsigned long deleted,
                                                   unsigned long create_revision, unsigned long prev_revision,
                                                   unsigned long lease,
                                                   std::string value, std::string old_value)
{

    // Put the values into a Byte vector
    std::vector<unsigned char> vec_value(value.begin(), value.end());
    std::vector<unsigned char> vec_old_value(old_value.begin(), old_value.end());
    return db_->expose_publish(id, name, created, deleted, create_revision, prev_revision, lease, vec_value, vec_old_value);
}

bool Controller::dispose(int id)
{
    return db_->expose_dispose(id);
}

QueryStructure::structure_massDisposeCompaction Controller::massDisposeCompaction(int param1, int param2) {
    return db_->expose_massDisposeCompaction(param1, param2);
}

std::vector<QueryStructure::structure_generic> Controller::query_generic(int query_identifier, int limit,
                                                                         std::string param1, std::string param2, std::string param3, std::string param4, std::string param5)
{
    // std::cout << ">>>> QUERY_GENERIC" << std::endl;
    return db_->expose_querygeneric(query_identifier, limit, param1, param2, param3, param4, param5);
}

QueryStructure::structure_query03 Controller::query_three(std::string param1, std::string param2)
{
    // std::cout << ">>>> QUERY_03" << std::endl;
    return db_->expose_query03(param1, param2);
}

unsigned long Controller::query_compactrevision()
{
    // std::cout << ">>>> QUERY_COMPACTREVISION" << std::endl;
    return db_->expose_querycompactrevision();
}

unsigned long Controller::query_maximumid()
{
    // std::cout << ">>>> QUERY_MAXIMUMID" << std::endl;
    return db_->expose_querymaximumid().first;
}
