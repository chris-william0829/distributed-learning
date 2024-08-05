#include <iostream>
#include <cstdlib>
#include <string>
#include <mutex>
#include <atomic>
#include <csignal>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "raftRpc.pb.h"
#include "raftRpc.grpc.pb.h"


struct LogEntry {
    int term;
    std::string command;
};

class RaftNode {
    public:
        int id;
        int currentTerm;
        int votedFor;
        int commitIndex;
        int lastApplied;
        std::vector<int> nextIndex; // 记录领导者发送给每个跟随者的下一个日志条目索引
        std::vector<int> matchIndex; // 记录每个跟随者复制的最大日志条目索引
        std::vector<std::unique_ptr<raftRpc::RaftRpcService::Stub>> peers; // gRPC stubs for other nodes
        std::string state;
        std::mutex mtx;
        std::vector<LogEntry> log;

        bool handleRequestVote(const raftRpc::RequestVoteRequest* request);
        bool handleAppendEntries(const raftRpc::AppendEntriesRequest* request);
};

bool RaftNode::handleRequestVote(const raftRpc::RequestVoteRequest* request){
    if(request->term() > currentTerm) {
        currentTerm = request->term();
        votedFor = -1;
        state = "Follower";
    }

    if((votedFor == -1 || votedFor == request->candidateid()) &&
        (request->lastlogterm() > log.back().term ||
        (request->lastlogterm() == log.back().term && request->lastlogindex() >= log.size() - 1))){
            votedFor = request->candidateid();
            return true;
    }
    return false;
}

bool RaftNode::handleAppendEntries(const raftRpc::AppendEntriesRequest* request){
    if (request->term() < currentTerm) {
        return false;
    }
    if (log.size() <= request->prevlogindex() || log[request->prevlogindex()].term != request->prevlogterm()) {
        return false;
    }
    log.erase(log.begin() + request->prevlogindex() + 1, log.end());
    log.insert(log.end(), request->entries().begin(), request->entries().end());

    if (request->leadercommit() > commitIndex) {
        commitIndex = std::min(request->leadercommit(), static_cast<int>(log.size() - 1));
    }

    return true;
}

class RaftRpcServiceImpl final : public raftRpc::RaftRpcService::Service {
    public:
        RaftNode* node;
        RaftRpcServiceImpl(RaftNode* node) : node(node) {}
        grpc::Status RequestVote(grpc::ServerContext* context, const raftRpc::RequestVoteRequest* request, raftRpc::RequestVoteResponse* response) override {
            std::unique_lock<std::mutex> lock(node->mtx);
            response->set_term(node->currentTerm);
            response->set_votegranted(node->handleRequestVote(request));
        }

        grpc::Status AppendEntries(grpc::ServerContext* context, const raftRpc::AppendEntriesRequest* request, raftRpc::AppendEntriesResponse* response) override {
            std::unique_lock<std::mutex> lock(node->mtx);
            response->set_term(node->currentTerm);
            response->set_success(node->handleAppendEntries(request));
            return grpc::Status::OK;
        }
};