# Compile using g flag
# SET(GCC_COMPILE_FLAGS "-g -frecord-gcc-switches")
# add_definitions(${GCC_COMPILE_FLAGS})
# Decomment to use asan for memory debugging
# add_compile_options(-fsanitize=address)
# add_link_options(-fsanitize=address)

# CMake file for the Database
project(Database LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.8)

if(NOT TARGET CycloneDDS-CXX::ddscxx)
    find_package(CycloneDDS-CXX REQUIRED)
endif()

# Protobuf
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

# Protobuf-compiler
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)


# Proto file
get_filename_component(hw_proto "Communication.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
message("Generating Protobuf and gRPC sources")
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/Communication.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/Communication.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/Communication.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/Communication.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")
message("Finished generating Protobuf and gRPC sources")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

idlcxx_generate(TARGET databasedata FILES ./src/idl/DatabaseData.idl WARNINGS no-implicit-extensibility)

# Create the executable, linking the code files that need to be used
add_executable(Database ./src/main.cpp ./src/Controller.cpp ./src/Database.cpp ./src/Communication.grpc.pb.cc ./src/Communication.pb.cc)

# Link both executables to idl data type library and ddscxx.
target_link_libraries(Database 
    CycloneDDS-CXX::ddscxx 
    databasedata
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})


# Disable the static analyzer in GCC to avoid crashing the GNU C++ compiler
# on Azure Pipelines
if(DEFINED ENV{SYSTEM_TEAMFOUNDATIONSERVERURI})
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND ANALYZER STREQUAL "on")
        target_compile_options(Database PRIVATE -fno-analyzer -g)
    endif()
endif()

set_property(TARGET Database PROPERTY CXX_STANDARD ${cyclonedds_cpp_std_to_use})