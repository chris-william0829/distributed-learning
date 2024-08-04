#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "log.hpp"
#include "node.hpp"
#include "network.hpp"

enum State {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

class RaftNode {
public:
    RaftNode(int id, int totalNodes) : nodeId(id), totalNodes(totalNodes), state(FOLLOWER), stop(false) {}

    void start() {
        electionThread = std::thread(&RaftNode::runElectionTimeout, this);
        heartbeatThread = std::thread(&RaftNode::runHeartbeatTimeout, this);
    }

    void stopNode() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        if (electionThread.joinable()) electionThread.join();
        if (heartbeatThread.joinable()) heartbeatThread.join();
    }

    ~RaftNode() {
        stopNode();
    }

    bool isLeader() const {
        return state == LEADER;
    }

    void handleClientRequest(const std::string& command) {
        std::lock_guard<std::mutex> lock(mtx);
        if (isLeader()) {
            LogEntry entry = { currentTerm, command };
            log.push_back(entry);
            network.broadcastMessage(entry); 
        } else {
            // Redirect to the current leader
        }
    }

private:
    int nodeId;
    int totalNodes;
    std::atomic<int> currentTerm;
    int votedFor;
    std::vector<LogEntry> log;
    State state;

    std::mutex mtx;
    std::condition_variable cv;
    bool stop;
    std::thread electionThread;
    std::thread heartbeatThread;

    Network network;

    void runElectionTimeout() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_for(lock, std::chrono::milliseconds(getRandomTimeout()), [this]() { return stop; })) {
                break;  // Stop was called
            }
            // Election timeout logic
        }
    }

    void runHeartbeatTimeout() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);
            if (cv.wait_for(lock, std::chrono::milliseconds(heartbeatInterval()), [this]() { return stop; })) {
                break;  // Stop was called
            }
            if (state == LEADER) {
                sendHeartbeat();
            }
        }
    }

    int getRandomTimeout() {
        // Return a random timeout value
        return 150 + rand() % 150; // Example: 150ms to 300ms
    }

    int heartbeatInterval() {
        return 50; // Example: 50ms
    }

    void sendHeartbeat() {
        std::cout << "Sending heartbeat" << std::endl;
        // Implementation of sending heartbeat
    }

    // Other methods for election and log replication
};
