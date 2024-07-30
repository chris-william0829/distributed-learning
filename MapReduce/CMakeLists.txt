cmake_minimum_required(VERSION 3.5)
project(MapReduce)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_library(mrFunc SHARED MapReduceFunc.cpp)


add_executable(master master.cpp MapReduce.grpc.pb.cc MapReduce.pb.cc)
target_link_libraries(master gRPC::grpc++ protobuf::libprotobuf)

add_executable(worker worker.cpp MapReduce.grpc.pb.cc MapReduce.pb.cc)
target_link_libraries(worker gRPC::grpc++ protobuf::libprotobuf mrFunc)
