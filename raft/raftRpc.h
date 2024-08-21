#include <iostream>
#include <cstdlib>
#include <string>
#include <mutex>
#include <atomic>
#include <csignal>
#include <unordered_map>
#include <chrono>
#include <future>
#include <vector>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "raftRpc.pb.h"
#include "raftRpc.grpc.pb.h"
#include "ApplyMsg.h"


struct LogEnt {
    int term;
    std::string command;
};

ApplyMsg parseLogEntry(const LogEnt& entry);

class RaftNode {
    public:
        int id;
        int currentTerm;
        int votedFor;
        int commitIndex;
        int lastApplied;
        std::string address;
        std::vector<int> nextIndex; // 记录领导者发送给每个跟随者的下一个日志条目索引
        std::vector<int> matchIndex; // 记录每个跟随者复制的最大日志条目索引
        std::vector<std::unique_ptr<raftRpc::RaftRpcService::Stub>> peers; // gRPC stubs for other nodes
        std::string state;
        std::mutex mtx;
        std::vector<LogEnt> log;
        std::function<bool(const ApplyMsg&)> applyCh;
        std::vector<std::promise<bool>*> pendingPromises;
        std::atomic<bool> running{true};
        std::condition_variable cv; // 用于等待和通知
        std::chrono::steady_clock::time_point lastHeartbeatTime; // 上一次心跳时间
        RaftNode(int id, std::string address);
        ~RaftNode();
        void setApplyCh(std::function<bool(const ApplyMsg&)> ch);
        void startElection();
        void appendEntries();
        void sendAppendEntries(int peerIndex);
        void tryCommitEntries();
        int start(const std::string& command, std::promise<bool>& resultPromise);
        bool handleRequestVote(const raftRpc::RequestVoteRequest* request);
        bool handleAppendEntries(const raftRpc::AppendEntriesRequest* request);
        void heartbeat();
        void electionTimeout();
        void initPeers(const std::vector<std::string>& peerAddresses);
        void updateLastHeartbeatTime();
    private:
        int HEARTBEAT_INTERVAL_MS = 100;
        int ELECTION_TIMEOUT_MS = 150;

};


class RaftRpcServiceImpl final : public raftRpc::RaftRpcService::Service {
    public:
        RaftNode* node;
        RaftRpcServiceImpl(RaftNode* node) : node(node) {}
        grpc::Status RequestVote(grpc::ServerContext* context, const raftRpc::RequestVoteRequest* request, raftRpc::RequestVoteResponse* response) override {
            std::unique_lock<std::mutex> lock(node->mtx);
            response->set_term(node->currentTerm);
            response->set_votegranted(node->handleRequestVote(request));
            return grpc::Status::OK;
        }

        grpc::Status AppendEntries(grpc::ServerContext* context, const raftRpc::AppendEntriesRequest* request, raftRpc::AppendEntriesResponse* response) override {
            std::unique_lock<std::mutex> lock(node->mtx);
            response->set_term(node->currentTerm);
            response->set_success(node->handleAppendEntries(request));
            return grpc::Status::OK;
        }
};