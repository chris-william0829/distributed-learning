#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<mutex>
#include<unordered_map>
#include<queue>
#include<chrono>
#include <grpcpp/grpcpp.h>
#include"MapReduce.pb.h"
#include"MapReduce.grpc.pb.h"

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
};


class Master {
    private:
        std::mutex mtx;
        std::unordered_map<int, Task*> tasksToDo;
        std::unordered_map<int, Task*> tasksDoing;
    public:
        static int taskNum;
        Master();
        void getTasks(char *file[], int num);
        void handlePreviousTask(const AckAndQueryNewTaskRequest* request);
        class TaskServiceImpl final : public TaskService::Service {
            status AckAndQueryNewTask(ServerContext* context, const AckAndQueryNewTaskRequest* request,
             AckAndQueryNewTaskResponse* reply) override {
                if(request->taskType != ""){
                    handlePreviousTask(request);
                }
                return Status::OK;
        }
    };
};

int Master::taskNum = 0;

void Master::getTasks(char *file[], int num) {
    for(int i = 1; i < num; i++) {
        Task* task = new Task();
        task->taskId = taskNum++;
        task->taskType = "map";
        task->filename = file[i];
        tasksToDo[task->taskId] = task;
    }
}

void Master::handlePreviousTask(const AckAndQueryNewTaskRequest* request) {
    std::unique_lock<std::mutex> lock(mtx);
    if(tasksDoing.find(request->previousTaskId) != tasksDoing.end()) {
        Task *task = tasksDoing[request->previousTaskId];
        if(task->workerId == request->workerId){
            std::cout << task->taskType << "task" << task->taskId << "finished on worker" << task->workerId << std::endl;
            if(task->taskType == "map"){
                
            }
        }
    }else{
        std::cout << "the task is not in the doing queue" << std::endl;
        return;
    }
}
