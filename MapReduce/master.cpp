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
#include "MapReduce.pb.h"
#include "MapReduce.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using MapReduce::AckAndQueryNewTaskRequest;
using MapReduce::AckAndQueryNewTaskResponse;
using MapReduce::TaskService;


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


class Task {
public:
    int taskId;
    std::string taskType;
    std::string filename;
    std::string workerId;
    std::chrono::high_resolution_clock::time_point deadline;

    Task() : taskId(0), taskType(""), filename(""), workerId("") {}
};

class Master {
public:
    std::mutex mtx;
    std::unordered_map<int, std::unique_ptr<Task>> tasksToDo;
    std::unordered_map<int, std::unique_ptr<Task>> tasksDoing;
    bool done;
    static int mapTaskCnt;
    static int reduceTaskCnt;
    std::string state;

    Master();
    Master(int _mapCnt, int _reduceCnt);
    void getMapTasks(char* file[], int num);
    void getReduceTasks();
    void handlePreviousTask(const AckAndQueryNewTaskRequest* request);
    void transit();
    Task* getAvailableTask();
};

int Master::mapTaskCnt = 0;
int Master::reduceTaskCnt = 0;

Master::Master(){
    done = false;
    state = "map";
}

void Master::getMapTasks(char* file[], int num) {
    for (int i = 1; i < num; i++) {
        auto task = std::make_unique<Task>();
        task->taskId = mapTaskCnt++;
        task->taskType = "map";
        task->filename = file[i];
        tasksToDo[task->taskId] = std::move(task);
    }
    return;
}

void Master::getReduceTasks(){
    for(int i = 0; i < mapTaskCnt; i++){
        auto task = std::make_unique<Task>();
        task->taskId = reduceTaskCnt++;
        task->taskType = "reduce";
        task->filename = "";
        tasksToDo[task->taskId] = std::move(task);
    }
    return;
}

void Master::transit(){
    if(state == "map"){
        std::cout << "all map-type tasks finished, transit to reduce phase." << std::endl;
        state = "reduce";
        getReduceTasks();
    }else if(state == "reduce"){
        std::cout << "all reduce-type tasks finished, prepare to exit!" << std::endl;
        state = "finished";
    }
    return;
}

std::string generateFilename(const int taskId, const std::string taskType) {
    return taskType + "_out_" + std::to_string(taskId);
}

void Master::handlePreviousTask(const AckAndQueryNewTaskRequest* request) {
    std::unique_lock<std::mutex> lock(mtx);
    auto it = tasksDoing.find(request->previoustaskid());
    if (it != tasksDoing.end()) {
        Task* task = it->second.get();
        if (task->workerId == request->workerid()) {
            tasksDoing.erase(it);
            if (task->taskType == "map") {
                std::cout << "map-task:" << task->taskId << " finished on worker " << task->workerId << std::endl;
                std::string tempMapFilename = tempMapOutFile(task->workerId, task->taskId);
                std::string FinalMapFilename = finalMapOutFile(task->taskId);
                if (std::rename(tempMapFilename.c_str(), FinalMapFilename.c_str()) == 0) {
                    std::cout << "Task commit successfully." << std::endl;
                } else {
                    std::perror("Error Committing");
                }
                if(tasksDoing.empty() && tasksToDo.empty()){
                    transit();
                }
            } else if (task->taskType == "reduce") {
                std::cout << "reduce-task:" << task->taskId << " finished on worker " << task->workerId << std::endl;
                std::string tempReduceFilename = tempReduceOutFile(task->workerId, task->taskId);
                std::string FinalReduceFilename = finalReduceOutFile(task->taskId);
                if (std::rename(tempReduceFilename.c_str(), FinalReduceFilename.c_str()) == 0) {
                    std::cout << "Task commit successfully." << std::endl;
                } else {
                    std::perror("Error Committing");
                }
                if (tasksDoing.empty() && tasksToDo.empty()) {
                    transit();
                }
            }
        } else {
            std::cout << task->taskType << " task " << task->taskId << " is no longer belongs to this worker" << std::endl;
        }
    } else {
        std::cout << "The task is not in the doing queue" << std::endl;
    }
}

Task* Master::getAvailableTask() {
    std::unique_lock<std::mutex> lock(mtx);
    if(state == "map" || state == "reduce"){
        if (!tasksToDo.empty()) {
            auto it = tasksToDo.begin();
            Task* task = it->second.get();
            tasksToDo.erase(it);
            return task;
        }else{
            return nullptr;
        }
    }else if(state == "finished"){
        Task *task = new Task();
        task->taskType = "finish";
        task->taskId = -1;
        task->filename = "";
        return task;
    }
}

class TaskServiceImpl final : public TaskService::Service {
public:
    Master* master;

    Status AckAndQueryNewTask(ServerContext* context, const AckAndQueryNewTaskRequest* request,
        AckAndQueryNewTaskResponse* reply) override {
        // Step 1: Handle previous task if necessary
        if (!request->tasktype().empty()) {
            master->handlePreviousTask(request);
        }
        // Step 2: Assign a new task
        Task* task = master->getAvailableTask();
        if (task != nullptr) {
            std::unique_lock<std::mutex> lock(master->mtx);
            task->workerId = request->workerid();
            task->deadline = std::chrono::high_resolution_clock::now() + std::chrono::seconds(10);
            master->tasksDoing[task->taskId] = std::make_unique<Task>(*task);
            reply->set_taskid(task->taskId);
            reply->set_filename(task->filename);
            reply->set_tasktype(task->taskType);
            return Status::OK;
        } else {
            return Status::CANCELLED;
        }
    }
};

void checkWorkers(Master* master) {
    std::unique_lock<std::mutex> lock(master->mtx);
    for (auto it = master->tasksDoing.begin(); it != master->tasksDoing.end(); ) {
        Task* task = it->second.get();
        auto now = std::chrono::high_resolution_clock::now();
        if (now > task->deadline) {
            std::cout << "Task " << task->taskId << " is timeout" << std::endl;
            task->workerId = "";
            it = master->tasksDoing.erase(it); // Update iterator
        } else {
            ++it; // Continue to the next element
        }
    }
}

void RunServer(Master* master) {
    std::string server_address("0.0.0.0:50051");
    TaskServiceImpl service;
    service.master = master;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

// 使用 std::atomic 变量来安全地控制多个线程之间的退出标志
std::atomic<bool> stopFlag(false);

void signalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    // 设置标志位来停止循环
    stopFlag = true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Missing parameter! The format is ./Master pg*.txt" << std::endl;
        return -1;
    }
    Master* master = new Master();
    master->getMapTasks(argv, argc);
    RunServer(master);
    std::thread checkWorkersThread;
    while (stopFlag) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        checkWorkersThread = std::thread(checkWorkers, master);
        checkWorkersThread.join();
    }

    delete master;
    return 0;
}
