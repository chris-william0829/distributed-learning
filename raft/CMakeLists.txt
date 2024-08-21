cmake_minimum_required(VERSION 3.5)
project(raft)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)



add_executable(server server.cpp raftRpc.cpp KVStore.grpc.pb.cc KVStore.pb.cc raftRpc.grpc.pb.cc raftRpc.pb.cc)
target_link_libraries(server gRPC::grpc++ protobuf::libprotobuf)
