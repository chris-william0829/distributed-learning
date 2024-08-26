cmake_minimum_required(VERSION 3.5)
project(raft)

find_package(gRPC CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

# 添加 gRPC 和 Protobuf 的包含路径
include_directories(${GRPC_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})

# 设置编译选项，减少优化级别并关闭调试信息
set(CMAKE_CXX_FLAGS_RELEASE "-O1")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")

# 编译 Raft RPC 静态库
add_library(raft_rpc STATIC raftRpc.cpp raftRpc.grpc.pb.cc raftRpc.pb.cc)

# 编译 Key-Value Store Proto 静态库
add_library(kv_store_proto STATIC KVStore.grpc.pb.cc KVStore.pb.cc)

# 编译 server 可执行文件
add_executable(server server.cpp)
target_link_libraries(server raft_rpc kv_store_proto gRPC::grpc++ protobuf::libprotobuf)

# 编译 client 可执行文件
add_executable(client client.cpp)
target_link_libraries(client kv_store_proto gRPC::grpc++ protobuf::libprotobuf)

# 减少链接时的内存使用
set(CMAKE_LINKER_FLAGS "-Wl,--no-keep-memory")
