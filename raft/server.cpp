#include <iostream>
#include <grpcpp/grpcpp.h>
#include "rpc.grpc.pb.h"
#include "kvstore/store.hpp"
#include "raft/raft.hpp"

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
