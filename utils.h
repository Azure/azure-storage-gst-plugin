#include <storage_outcome.h>

using namespace azure::storage_lite;

// error handling
template <typename T>
void handle(storage_outcome<T> &outcome) {
    if(outcome.success()) {
        std::cout << "Request succeeded." << std::endl;
    } else {
        std::cout << "Request failed." << std::endl;
        std::cout << outcome.error().code << '(' << outcome.error().code_name << ')' << std::endl;
        std::cout << outcome.error().message << std::endl;
    }
}