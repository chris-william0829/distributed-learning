#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string>
#include<mutex>
#include<unordered_map>
#include <grpcpp/grpcpp.h>
#include"MapReduce.pb.h"
#include"MapReduce.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using MapReduce::Task;
using MapReduce::AckAndQueryNewTaskRequest;
using MapReduce::AckAndQueryNewTaskResponse;
using MapReduce::TaskService;

class Master{
    private:
        std::mutex mtx;
        std::string taskState;
        int mapWorkerCnt;
        int reduceWorkerCnt;
        std::map<
};