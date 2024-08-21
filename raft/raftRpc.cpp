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


ApplyMsg parseLogEntry(const LogEntry& entry) {
    ApplyMsg msg;
    size_t pos = entry.command.find(":");
    msg.key = entry.command.substr(0, pos);
    msg.value = entry.command.substr(pos + 1);
    return msg;
}

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
        std::vector<std::promise<bool>*> pendingPromises;

        void setApplyCh(std::function<bool(const ApplyMsg&)> ch);
        void startElection();
        void appendEntries();
        void sendAppendEntries(int peerIndex);
        void tryCommitEntries();
        int start(const std::string& command, std::promise<bool>& resultPromise);
        bool handleRequestVote(const raftRpc::RequestVoteRequest* request);
        bool handleAppendEntries(const raftRpc::AppendEntriesRequest* request);

};

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

    // 等待所有异步任务完成
    for (auto& future : futures) {
        future.get();
    }

    tryCommitEntries();

}

int RaftNode::start(const std::string& command, std::promise<bool>& resultPromise){
    if(state != "Leader"){
        return votedFor;
    }
    std::unique_lock<std::mutex> lock(mtx);
    log.push_back({currentTerm, command});
    pendingPromises[log.size()-1] = &resultPromise;
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