#include <iostream>
#include <cstdlib>
#include <string>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <atomic>
#include <dlfcn.h>
#include <csignal>
#include <vector>
#include "KeyValue.h"
#include <grpcpp/grpcpp.h>
#include "MapReduce.pb.h"
#include "MapReduce.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using MapReduce::AckAndQueryNewTaskRequest;
using MapReduce::AckAndQueryNewTaskResponse;
using MapReduce::TaskService;


typedef std::vector<KeyValue> (*MapF)(std::string);
typedef std::vector<KeyValue> (*ReduceF)(std::unordered_map<std::string, std::string>);

MapF mapFunc;
ReduceF reduceFunc;

int mapCnt;
int reduceCnt;

class TaskServiceClient {
    public:
        TaskServiceClient(std::shared_ptr<Channel> channel)
          : stub_(TaskService::NewStub(channel)) {}
        
        AckAndQueryNewTaskResponse* AckAndQueryNewTask(AckAndQueryNewTaskRequest request){
            AckAndQueryNewTaskResponse* response = new AckAndQueryNewTaskResponse();
            ClientContext context;
            Status status = stub_->AckAndQueryNewTask(&context, request, response);
            if (status.ok()) {
                return response;
            }else{
                std::cout << "gRPC call failed: " << status.error_message() << std::endl;
                return nullptr;
            }
        }

    private:
        std::unique_ptr<TaskService::Stub> stub_;
};

int ihash(const std::string& key) {
    std::hash<std::string> hasher;
    size_t hash_val = hasher(key);

    // 使用与操作确保结果为正数
    return static_cast<int>(hash_val & 0x7fffffff);
}

std::string generateWorkerId(){
    std::ostringstream oss;
    oss << std::this_thread::get_id();  // 使用线程ID
    return oss.str();
}

std::string tempMapOutFile(std::string workerId, int taskId){
    return "map-" + workerId + "-" + std::to_string(taskId);
}

std::string finalMapOutFile(int taskId){
    return "map-out-" + std::to_string(taskId);
}

std::string tempReduceOutFile(std::string workerId, int taskId){
    return "reduce-" + workerId + "-" + std::to_string(taskId);
}

std::string finalReduceOutFile(int taskId){
    return "out-" + std::to_string(taskId);
}

void worker(){
    std::string workerId = generateWorkerId();
    std::cout << "Create Worker: " << workerId << std::endl;

    std::string taskType = "";
    int previousTaskId = -1;

    TaskServiceClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    std::cout << "create channel" << std::endl;
    while(1){
        AckAndQueryNewTaskRequest request;
        request.set_previoustaskid(previousTaskId);
        request.set_workerid(workerId);
        request.set_tasktype(taskType);
        
        AckAndQueryNewTaskResponse *response = client.AckAndQueryNewTask(request);
        if(response == nullptr){
            std::cerr << workerId <<": Failed to call AckAndQueryNewTask, retry 1 second later" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        if(response->tasktype() == "map"){
            std::vector<KeyValue> kvs = mapFunc(response->filename());
            
            std::string tempMapOutFilename = tempMapOutFile(workerId, response->taskid());
            std::ofstream tempMapOutFile(tempMapOutFilename, std::ios::app);
            if (!tempMapOutFile) {
                std::cerr << "Error: Could not open the file for writing." << std::endl;
                return;
            }
            for(auto kv : kvs){
                std::string content = kv.key + " " + kv.value + "\n";
                tempMapOutFile << content;
            }
            tempMapOutFile.close();

        }

        if(response->tasktype() == "reduce"){
            int taskId = response->taskid();
            std::unordered_map<std::string, std::string> bucket;
            for(int i = 0; i < mapCnt; i++){
                std::string mapOutFilename = finalMapOutFile(i);
                std::ifstream mapOutFile(mapOutFilename);
                if (!mapOutFile.is_open()) {
                    std::cerr << "Failed to open the map file." << std::endl;
                    return;
                }
                std::string line;
                while (std::getline(mapOutFile, line)) { // 逐行读取文件内容
                    std::istringstream spilt(line);
                    std::string key, value;
                    if (spilt >> key >> value) {
                        if((ihash(key) % reduceCnt) == taskId){
                            if(bucket.find(key) != bucket.end()){
                                bucket[key].append("1");
                            }else{
                                bucket[key] = value;
                            }
                        }
                    } else {
                        std::cerr << "Line format is incorrect: " << line << std::endl;
                    }
                }
                mapOutFile.close();
            }
            std::vector<KeyValue> kvs = reduceFunc(bucket);
            std::sort(kvs.begin(), kvs.end(), [](const KeyValue& a, const KeyValue& b) {
                return a.key < b.key;
            });
            std::string tempReduceOutFilename = tempReduceOutFile(workerId, taskId);
            std::ofstream tempReduceOutFile(tempReduceOutFilename, std::ios::app);
            if (!tempReduceOutFile) {
                std::cerr << "Error: Could not open the file for writing." << std::endl;
                return;
            }
            for(auto kv : kvs){
                std::string content = kv.key + " " + kv.value + "\n";
                tempReduceOutFile << content;
            }
            tempReduceOutFile.close();
        }

        if(response->tasktype() == "finish"){
            break;
        }
    }
    return;
}

// 使用 std::atomic 变量来安全地控制多个线程之间的退出标志
std::atomic<bool> stopFlag(false);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    // 设置标志位来停止循环
    stopFlag = true;
}

int main(){

    mapCnt = 8;
    reduceCnt = 8;

    void* handle = dlopen("./libmrFunc.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Cannot open library: " << dlerror() << std::endl;
        exit(-1);
    }
    mapFunc = (MapF)dlsym(handle, "mapFunc");
    if (!mapFunc) {
        std::cerr << "Cannot load symbol 'mapFunc': " << dlerror() << std::endl;
        dlclose(handle);
        exit(-1);
    }
    reduceFunc = (ReduceF)dlsym(handle, "reduceFunc");
    if (!reduceFunc) {
        std::cerr << "Cannot load symbol 'reduceFunc': " << dlerror() << std::endl;
        dlclose(handle);
        exit(-1);
    }

    for(int i = 0; i < mapCnt; i++){
        std::thread w(worker);
        w.detach();
    }

    signal(SIGINT, signalHandler);
    while (!stopFlag.load()) {        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    dlclose(handle);
    return 0;
}