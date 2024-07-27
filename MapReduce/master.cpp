#include <iostream>
#include <cstdlib>
#include <string>
#include <mutex>
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
    static int taskNum;

    Master();
    void getTasks(char* file[], int num);
    std::string generateFilename(int taskId, const std::string& taskType);
    void handlePreviousTask(const AckAndQueryNewTaskRequest* request);
    Task* getAvailableTask();
};

int Master::taskNum = 0;

Master::Master() : done(false) {}

void Master::getTasks(char* file[], int num) {
    for (int i = 1; i < num; i++) {
        auto task = std::make_unique<Task>();
        task->taskId = taskNum++;
        task->taskType = "map";
        task->filename = file[i];
        tasksToDo[task->taskId] = std::move(task);
    }
}

std::string Master::generateFilename(int taskId, const std::string& taskType) {
    return taskType + "_out_" + std::to_string(taskId);
}

void Master::handlePreviousTask(const AckAndQueryNewTaskRequest* request) {
    std::unique_lock<std::mutex> lock(mtx);
    auto it = tasksDoing.find(request->previoustaskid());
    if (it != tasksDoing.end()) {
        Task* task = it->second.get();
        if (task->workerId == request->workerid()) {
            std::cout << task->taskType << " task " << task->taskId << " finished on worker " << task->workerId << std::endl;
            tasksDoing.erase(it);
            if (task->taskType == "map") {
                task->filename = generateFilename(task->taskId, task->taskType);
                task->taskType = "reduce";
                tasksToDo[task->taskId] = std::move(it->second);
            } else if (task->taskType == "reduce") {
                if (tasksDoing.empty() && tasksToDo.empty()) {
                    std::cout << "All tasks finished" << std::endl;
                    done = true;
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
    if (!tasksToDo.empty()) {
        auto it = tasksToDo.begin();
        Task* task = it->second.get();
        tasksToDo.erase(it);
        return task;
    }
    return nullptr;
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Missing parameter! The format is ./Master pg*.txt" << std::endl;
        return -1;
    }
    Master* master = new Master();
    master->getTasks(argv, argc);
    RunServer(master);
    std::thread checkWorkersThread;
    while (!master->done) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        checkWorkersThread = std::thread(checkWorkers, master);
        checkWorkersThread.join();
    }

    delete master;
    return 0;
}
