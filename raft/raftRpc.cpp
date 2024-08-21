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

ApplyMsg parseLogEntry(const LogEnt& entry) {
    ApplyMsg msg;
    
    // 找到分隔符的位置
    size_t pos = entry.command.find(":");

    // 检查是否找到了分隔符
    if (pos == std::string::npos) {
        // 处理错误情况，例如记录日志或抛出异常
        std::cerr << "Invalid command format in LogEntry: " << entry.command << std::endl;
        // 设置默认值或者抛出异常
        msg.key = "";
        msg.value = "";
        return msg;
    }

    // 提取 key 和 value
    msg.key = entry.command.substr(0, pos);
    msg.value = entry.command.substr(pos + 1);

    return msg;
}


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



void RaftNode::heartbeat(){
    while(running){
        std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));
        if (state == "Leader") {
            appendEntries();  // 发送心跳
        }
    }
}

void RaftNode::electionTimeout() {
    std::unique_lock<std::mutex> lock(mtx);
    while (running) {
        auto now = std::chrono::steady_clock::now();
        auto timeout = std::chrono::milliseconds(ELECTION_TIMEOUT_MS);
        cv.wait_until(lock, lastHeartbeatTime + timeout, [this]() {
            return !running || std::chrono::steady_clock::now() >= lastHeartbeatTime + std::chrono::milliseconds(ELECTION_TIMEOUT_MS);
        });

        if (running && std::chrono::steady_clock::now() >= lastHeartbeatTime + timeout) {
            if (state == "Follower" || state == "Candidate") {
                startElection();  // 发起选举
            }
        }
    }
}

void RaftNode::updateLastHeartbeatTime() {
    std::unique_lock<std::mutex> lock(mtx);
    lastHeartbeatTime = std::chrono::steady_clock::now();
    cv.notify_all(); // 通知等待的线程
}

RaftNode::RaftNode(int id, std::string address) 
    : id(id), currentTerm(0), votedFor(-1), commitIndex(0), lastApplied(0), address(address), state("Follower") {
    lastHeartbeatTime = std::chrono::steady_clock::now();
}

RaftNode::~RaftNode(){
    running = false;
}

// 初始化 peers
void RaftNode::initPeers(const std::vector<std::string>& peerAddresses) {
    for (size_t i = 0; i < peerAddresses.size(); ++i) {
        if (peerAddresses[i] != address) { // 不包括自己
            peers[i] = raftRpc::RaftRpcService::NewStub(
                grpc::CreateChannel(peerAddresses[i], grpc::InsecureChannelCredentials()));
        }
    }
}

void RaftNode::setApplyCh(std::function<bool(const ApplyMsg&)> ch) {
    applyCh = ch;
}

void RaftNode::startElection() {
    std::unique_lock<std::mutex> lock(mtx);
    currentTerm++;
    state = "Candidate";
    votedFor = id;

    int votes = 1; // Vote for self

    for (auto& peer : peers) {
        raftRpc::RequestVoteRequest request;
        request.set_term(currentTerm);
        request.set_candidateid(id);
        request.set_lastlogindex(log.size() - 1);
        request.set_lastlogterm(log.back().term);

        raftRpc::RequestVoteResponse response;
        grpc::ClientContext context;
        grpc::Status status = peer->RequestVote(&context, request, &response);

        if (status.ok() && response.votegranted()) {
            votes++;
        }

        if (votes > peers.size() / 2) {
            state = "Leader";
            // Initialize leader state
            nextIndex.assign(peers.size(), log.size());
            matchIndex.assign(peers.size(), 0);
            break;
        }
    }
}

void RaftNode::tryCommitEntries() {
    std::unique_lock<std::mutex> lock(mtx);

    std::vector<int> sortedMatchIndex = matchIndex;
    std::sort(sortedMatchIndex.begin(), sortedMatchIndex.end());

    // 找到大多数节点都有的日志条目索引
    int majorityIndex = sortedMatchIndex[(sortedMatchIndex.size() - 1) / 2];

    // 如果该日志条目的term是当前term，则提交
    if (majorityIndex > commitIndex && log[majorityIndex].term == currentTerm) {

        int lastCommitIndex = commitIndex;
        for (int i = lastCommitIndex + 1; i <= majorityIndex; ++i) {
                ApplyMsg msg = parseLogEntry(log[i]);

                // 处理日志条目，直到成功提交
                bool success = false;
                while (!success) {
                    if (applyCh) {
                        success = applyCh(msg);  // 调用回调函数
                    }

                    if (!success) {
                        // 如果处理失败，等待一段时间再重试
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }

                commitIndex = i;  // 更新 commitIndex
                pendingPromises[commitIndex]->set_value(true);
            }
    }
}

void RaftNode::sendAppendEntries(int peerIndex) {
    raftRpc::AppendEntriesRequest request;
    {
        std::lock_guard<std::mutex> lock(mtx);
        request.set_term(currentTerm);
        request.set_leaderid(id);
        request.set_prevlogindex(nextIndex[peerIndex] - 1);
        request.set_prevlogterm(log[nextIndex[peerIndex] - 1].term);
        request.set_leadercommit(commitIndex);

        for (size_t j = nextIndex[peerIndex]; j < log.size(); ++j) {
            raftRpc::LogEntry* entry = request.add_entries();
            entry->set_index(j);
            entry->set_term(log[j].term);
            entry->set_command(log[j].command);
        }
    }

    raftRpc::AppendEntriesResponse response;
    grpc::ClientContext context;
    grpc::Status status = peers[peerIndex]->AppendEntries(&context, request, &response);
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (status.ok() && response.success()) {
            nextIndex[peerIndex] = log.size();
            matchIndex[peerIndex] = log.size() - 1;
        } else {
            nextIndex[peerIndex]--;
        }
    }
}

void RaftNode::appendEntries() {
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < peers.size(); ++i) {
        futures.push_back(std::async(std::launch::async, &RaftNode::sendAppendEntries, this, i));
    }

    for (auto& future : futures) {
        try {
            future.get();  // 等待异步任务完成
        } catch (const std::exception& e) {
            std::cerr << "Exception caught in appendEntries: " << e.what() << std::endl;
        }
    }

    tryCommitEntries();

}

int RaftNode::start(const std::string& command, std::promise<bool>& resultPromise){
    if(state != "Leader"){
        return votedFor;
    }
    std::unique_lock<std::mutex> lock(mtx);
    log.push_back({currentTerm, command});
    pendingPromises.push_back(&resultPromise);
    appendEntries();
    return id;
}


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

bool RaftNode::handleAppendEntries(const raftRpc::AppendEntriesRequest* request) {
    if (request->term() < currentTerm) {
        return false;
    }
    if (log.size() <= request->prevlogindex() || log[request->prevlogindex()].term != request->prevlogterm()) {
        return false;
    }
    log.erase(log.begin() + request->prevlogindex() + 1, log.end());

    // Convert Protobuf RepeatedPtrField to std::vector
    std::vector<LogEnt> newEntries;
    for (const auto& entry : request->entries()) {
        LogEnt logEntry;
        logEntry.term = entry.term();
        logEntry.command = entry.command();
        newEntries.push_back(logEntry);
    }

    log.insert(log.end(), newEntries.begin(), newEntries.end());

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
            return grpc::Status::OK;
        }

        grpc::Status AppendEntries(grpc::ServerContext* context, const raftRpc::AppendEntriesRequest* request, raftRpc::AppendEntriesResponse* response) override {
            std::unique_lock<std::mutex> lock(node->mtx);
            response->set_term(node->currentTerm);
            response->set_success(node->handleAppendEntries(request));
            return grpc::Status::OK;
        }
};