#include <iostream>
#include <grpcpp/grpcpp.h>
#include "rpc.grpc.pb.h"
#include "kvstore/store.hpp"
#include "raft/raft.hpp"


class KeyValueStore {
public:
    KeyValueStore(RaftNode* raftNode) : raftNode(raftNode) {
        // 设置 Raft 层的回调函数
        raftNode->applyCh = [this](const ApplyMsg& msg) {
            return this->applyLogEntry(msg);  // 直接处理日志条目并返回处理结果
        };
    }

    void put(const std::string& key, const std::string& value) {
        std::string request = key + ":" + value;
        raftNode->start(request);
    }

private:
    RaftNode* raftNode;
    std::unordered_map<std::string, std::string> kvStore;

    bool applyLogEntry(const ApplyMsg& msg) {
        try {
            kvStore[msg.key] = msg.value;
            std::cout << "Applied log entry: key=" << msg.key << ", value=" << msg.value << std::endl;
            return true;  // 处理成功
        } catch (...) {
            std::cerr << "Failed to apply log entry: key=" << msg.key << ", value=" << msg.value << std::endl;
            return false;  // 处理失败
        }
    }
};



class KVStoreServiceImpl final : public rpc::KVStore::Service {
public:



    KVStoreServiceImpl(RaftNode* raftNode, KVStore* store) : raftNode_(raftNode), store_(store) {}

    grpc::Status Get(grpc::ServerContext* context, const rpc::GetRequest* request, rpc::GetResponse* response) override {
        std::string value = store_->Get(request->key());
        response->set_value(value);
        return grpc::Status::OK;
    }

    grpc::Status Set(grpc::ServerContext* context, const rpc::SetRequest* request, rpc::SetResponse* response) override {
        if (raftNode_->isLeader()) {
            // Apply the command through Raft
            std::string command = "SET " + request->key() + " " + request->value();
            raftNode_->handleClientRequest(command);
            response->set_success(true);
        } else {
            // Redirect to the current leader (not implemented in this snippet)
            response->set_success(false);
        }
        return grpc::Status::OK;
    }

    grpc::Status Delete(grpc::ServerContext* context, const rpc::DeleteRequest* request, rpc::DeleteResponse* response) override {
        if (raftNode_->isLeader()) {
            // Apply the command through Raft
            std::string command = "DELETE " + request->key();
            raftNode_->handleClientRequest(command);
            response->set_success(true);
        } else {
            // Redirect to the current leader (not implemented in this snippet)
            response->set_success(false);
        }
        return grpc::Status::OK;
    }

private:
    RaftNode* raftNode_;
    KVStore* store_;
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    KVStore store;
    RaftNode raftNode(1, 3); // Example: node ID 1 in a 3-node cluster

    KVStoreServiceImpl service(&raftNode, &store);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    raftNode.start();
    server->Wait();
    raftNode.stopNode();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
