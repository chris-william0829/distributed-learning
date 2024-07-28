#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <bits/stdc++.h>


class KeyValue{
public:
    std::string key;
    std::string value;
};


extern "C" std::vector<KeyValue> mapFunc(std::string filename){
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
    }
    std::vector<KeyValue> kv;
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
                kv.emplace_back(tmp);
            }
        }
    }
    return kv;
}

extern "C" std::vector<string> reduceFunc(vector<KeyValue> kvs, int reduceTaskIdx){
    vector<string> str;
    string tmp;
    for(const auto& kv : kvs){
        str.push_back(to_string(kv.value.size()));
    }
    return str;
}