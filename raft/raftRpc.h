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
        std::function<bool(const ApplyMsg&)> applyCh;

        void setApplyCh(std::function<bool(const ApplyMsg&)> ch);
        void startElection();
        void appendEntries();
        void sendAppendEntries(int peerIndex);
        void tryCommitEntries();

        bool handleRequestVote(const raftRpc::RequestVoteRequest* request);
        bool handleAppendEntries(const raftRpc::AppendEntriesRequest* request);
        int start(const std::string& command, std::promise<bool>& resultPromise);
};