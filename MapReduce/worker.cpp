#include <iostream>
#include <cstdlib>
#include <string>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <sstream>
#include <unistd.h>
#include <grpcpp/grpcpp.h>
#include "MapReduce.pb.h"
#include "MapReduce.grpc.pb.h"



std::string generateWorkerId(){
    pid_t pid = getpid();
    std::ostringstream oss;
    oss << pid;
    return oss.str();
}

void* worker(void* argc){
    
}