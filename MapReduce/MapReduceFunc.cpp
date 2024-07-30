#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include<unordered_map>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bits/stdc++.h>
#include "KeyValue.h"


extern "C" std::vector<KeyValue> mapFunc(std::string filename){
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
    }
    std::vector<KeyValue> kvs;
    std::string line;
    while(std::getline(file, line)){
        std::stringstream lineStream(line);
        std::string word;
        while (lineStream >> word){
            std::string filteredWord;
            for (char c : word) {
                if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')){
                    filteredWord += c;
                }
            }
            if(!filteredWord.empty()){
                KeyValue tmp;
                tmp.key = filteredWord;
                tmp.value = "1";
                kvs.emplace_back(tmp);
            }
        }
    }
    return kvs;
}

extern "C" std::vector<KeyValue> reduceFunc(std::unordered_map<std::string, std::string> kvs){
    std::vector<KeyValue> result;
    for(const auto& kv : kvs){
        KeyValue temp;
        temp.key = kv.first;
        temp.value = std::to_string(kv.second.size());
        result.emplace_back(temp);
    }
    return result;
}