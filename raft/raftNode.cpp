#include "raft.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class RaftServiceImpl final : public raft::Raft::Service {
public:
    grpc::Status RequestVote(grpc::ServerContext* context, const raft::RequestVoteRequest* request, raft::RequestVoteResponse* response) override {
        std::cout << "Received RequestVote from: " << request->candidateid() << std::endl;
        response->set_term(request->term());
        response->set_votegranted(true);
        return grpc::Status::OK;
    }

    grpc::Status AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesRequest* request, raft::AppendEntriesResponse* response) override {
        std::cout << "Received AppendEntries from: " << request->leaderid() << std::endl;
        response->set_term(request->term());
        response->set_success(true);
        return grpc::Status::OK;
    }

    grpc::Status Heartbeat(grpc::ServerContext* context, const raft::HeartbeatRequest* request, raft::HeartbeatResponse* response) override {
        std::cout << "Received Heartbeat from: " << request->leaderid() << std::endl;
        response->set_term(request->term());
        response->set_success(true);
        return grpc::Status::OK;
    }
};

class RaftNode {
public:
    RaftNode(const std::string& address, const std::vector<std::string>& peer_addresses)
        : server_address_(address), peer_addresses_(peer_addresses) {
        for (const auto& peer : peer_addresses_) {
            stubs_.emplace_back(raft::Raft::NewStub(grpc::CreateChannel(peer, grpc::InsecureChannelCredentials())));
        }
    }

    void RunServer() {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address_, grpc::InsecureServerCredentials());
        builder.RegisterService(&service_);
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address_ << std::endl;
        server_->Wait();
    }

    void SendHeartbeat() {
        for (auto& stub : stubs_) {
            raft::HeartbeatRequest request;
            request.set_term(1);
            request.set_leaderid(1);
            raft::HeartbeatResponse response;
            grpc::ClientContext context;
            grpc::Status status = stub->Heartbeat(&context, request, &response);
            if (status.ok()) {
                std::cout << "Heartbeat success" << std::endl;
            } else {
                std::cout << "Heartbeat failed" << std::endl;
            }
        }
    }

private:
    std::string server_address_;
    std::vector<std::string> peer_addresses_;
    std::vector<std::unique_ptr<raft::Raft::Stub>> stubs_;
    RaftServiceImpl service_;
    std::unique_ptr<grpc::Server> server_;
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <server_address> <peer_address>..." << std::endl;
        return 1;
    }

    std::string server_address = argv[1];
    std::vector<std::string> peer_addresses;
    for (int i = 2; i < argc; ++i) {
        peer_addresses.push_back(argv[i]);
    }

    RaftNode node(server_address, peer_addresses);

    // Run server in a separate thread
    std::thread server_thread([&node]() { node.RunServer(); });

    // Simulate sending heartbeats
    while (true) {
        node.SendHeartbeat();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    server_thread.join();
    return 0;
}
