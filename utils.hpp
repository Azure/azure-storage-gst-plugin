#ifndef _AZURE_SINK_UTILS_HPP_
#define _AZURE_SINK_UTILS_HPP_

#include <iostream>
#include <storage_outcome.h>

using namespace azure::storage_lite;

// error handling
template <typename T>
void handle(storage_outcome<T> &outcome, std::ostream &out = std::cerr) {
    if(outcome.success()) {
        out << "Request succeeded." << std::endl;
    } else {
        out << "Request failed." << std::endl;
        out << outcome.error().code << '(' << outcome.error().code_name << ')' << std::endl;
        out << outcome.error().message << std::endl;
    }
}
#endif