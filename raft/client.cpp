#include <iostream>
#include <grpcpp/grpcpp.h>
#include "KVStore.grpc.pb.h"

void RunClient() {
    std::string target_str = "localhost:50051";
    auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
    std::unique_ptr<KVStore::KVStore::Stub> stub = KVStore::KVStore::NewStub(channel);

    while (true) {
        std::string command;
        std::cout << "Enter command (GET/SET key [value]): ";
        std::getline(std::cin, command);

        if (command.empty()) {
            continue;
        }

        std::istringstream iss(command);
        std::string operation;
        std::string key;
        std::string value;

        iss >> operation >> key;

        if (operation == "SET") {
            iss >> value;
            if (key.empty() || value.empty()) {
                std::cerr << "Invalid SET command format. Usage: SET key value" << std::endl;
                continue;
            }

            KVStore::SetRequest request;
            request.set_key(key);
            request.set_value(value);

            KVStore::SetResponse response;
            grpc::ClientContext context;
            grpc::Status status = stub->Set(&context, request, &response);

            if (status.ok() && response.success()) {
                std::cout << "SET operation successful." << std::endl;
            } else {
                std::cerr << "Failed to perform SET operation." << std::endl;
            }

        } else if (operation == "GET") {
            if (key.empty()) {
                std::cerr << "Invalid GET command format. Usage: GET key" << std::endl;
                continue;
            }

            KVStore::GetRequest request;
            request.set_key(key);

            KVStore::GetResponse response;
            grpc::ClientContext context;
            grpc::Status status = stub->Get(&context, request, &response);

            if (status.ok()) {
                std::cout << "Value: " << response.value() << std::endl;
            } else {
                std::cerr << "Failed to perform GET operation." << std::endl;
            }

        } else {
            std::cerr << "Unknown command. Use GET or SET." << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    RunClient();
    return 0;
}
