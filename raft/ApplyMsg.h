#ifndef APPLYMSG_H
#define APPLYMSG_H

#include<string>

struct ApplyMsg {
    std::string key;
    std::string value;
};

#endif