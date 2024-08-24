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
        std::cout << "Receive SET Operation: SET" << request->key() << " to " << request->value() << std::endl;
        std::promise<bool> resultPromise;
        auto resultFuture = resultPromise.get_future();

        // 向 Raft 节点发起请求
        while (true) {
            int leaderId = raftNodes[leaderIndex]->start(request->key() + ":" + request->value(), resultPromise);
            if (leaderId == leaderIndex) {
                std::cout << "Forward SET operation to raft layer" << std::endl;
                break;
            } else {
                std::cout << "raft leader changed" << std::endl;
                leaderIndex = leaderId;
            }
        }

        bool success = resultFuture.get();  // 等待 Raft 层响应
        std::cout << "SET SUCCESS" << std::endl;
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
    const std::vector<std::string> peerAddresses = {
        "0.0.0.0:50052", // 节点 1 的地址
        "0.0.0.0:50053", // 节点 2 的地址
        "0.0.0.0:50054"  // 节点 3 的地址
    };

    // 创建 Raft 节点
    std::vector<std::unique_ptr<RaftNode>> raftNodes;
    for (size_t i = 0; i < peerAddresses.size(); ++i) {
        raftNodes.push_back(std::make_unique<RaftNode>(i, peerAddresses[i]));
    }

    // 创建 KeyValueStore 服务实例
    KeyValueStore kvStoreService(raftNodes);

    // 启动每个 Raft 节点的 gRPC 服务器
    std::vector<std::unique_ptr<RaftRpcServiceImpl>> services;
    std::vector<std::unique_ptr<grpc::Server>> raftServers;

    for (size_t i = 0; i < raftNodes.size(); ++i) {
        auto& node = raftNodes[i];
        std::string server_address = peerAddresses[i];

        auto service = std::make_unique<RaftRpcServiceImpl>(node.get());
        services.push_back(std::move(service));

        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(services.back().get());

        auto server = builder.BuildAndStart();
        std::cout << "Raft node " << i << " listening on " << server_address << std::endl;
        raftServers.push_back(std::move(server));
    }

    sleep(6);

    for(auto& node : raftNodes){
        node->initPeers(peerAddresses);
    
        std::thread([node = node.get()]() {
            node->heartbeat();
        }).detach();
    
        std::thread([node = node.get()]() {
            node->electionTimeout();
        }).detach();
    }

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&kvStoreService);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 阻塞主线程，直到服务器关闭
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
