#include <iostream>
#include <vector>
#include <grpcpp/grpcpp.h>
#include "KVStore.grpc.pb.h"
#include "KVStore.pb.h"
#include "ApplyMsg.h"
#include "raftRpc.h"


class KeyValueStore final : public KVStore::KVStore::Service {
public:
    KeyValueStore(std::vector<std::unique_ptr<RaftNode>>& nodes)
        : raftNodes(nodes), leaderIndex(0) {
        // 初始化 applyCh 回调函数
        for (auto& node : raftNodes) {
            node->setApplyCh([this](const ApplyMsg& msg) {
                return this->applyLogEntry(msg);
            });
        }
    }

    grpc::Status Set(grpc::ServerContext* context, const KVStore::SetRequest* request, KVStore::SetResponse* response) override {
        std::promise<bool> resultPromise;
        auto resultFuture = resultPromise.get_future();

        // 向 Raft 节点发起请求
        while (true) {
            int leaderId = raftNodes[leaderIndex]->start(request->key() + ":" + request->value(), resultPromise);
            if (leaderId == leaderIndex) {
                break;
            } else {
                leaderIndex = leaderId;
            }
        }

        bool success = resultFuture.get();  // 等待 Raft 层响应
        response->set_success(success);
        return grpc::Status::OK;
    }

    grpc::Status Get(grpc::ServerContext* context, const KVStore::GetRequest* request, KVStore::GetResponse* response) override {
        auto it = kvStore.find(request->key());
        if (it != kvStore.end()) {
            response->set_value(it->second);
        } else {
            response->set_value("");  // 如果键不存在，返回空字符串
        }
        return grpc::Status::OK;
    }

private:
    std::vector<std::unique_ptr<RaftNode>>& raftNodes;
    std::unordered_map<std::string, std::string> kvStore;
    int leaderIndex;

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


void RunServer() {
    std::string server_address("0.0.0.0:50051");
    
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
