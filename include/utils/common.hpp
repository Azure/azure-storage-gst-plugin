#ifndef _AZURE_ELEMENTS_UTILS_COMMON_HPP_
#define _AZURE_ELEMENTS_UTILS_COMMON_HPP_

#include <iostream>
#include <istream>
#include <fstream>
#include <thread>
#include <storage_outcome.h>

using namespace azure::storage_lite;

// error handling
template <typename T>
inline void handle(storage_outcome<T> &outcome, std::ostream &out = std::cerr)
{
    if (outcome.success())
    {
        out << "Request succeeded." << std::endl;
    }
    else
    {
        out << "Request failed." << std::endl;
        out << outcome.error().code << '(' << outcome.error().code_name << ')' << std::endl;
        out << outcome.error().message << std::endl;
    }
}

// get the length of the content of a stream
// note that this is not thread-safe. You should not append
// content into the stream while calling this method.
template <class CharT>
inline unsigned int getStreamLen(std::basic_istream<CharT> &ss)
{
    auto cur = ss.tellg();
    ss.seekg(0, std::ios_base::end);
    auto end = ss.tellg();
    ss.seekg(cur);
    return end - cur;
}

inline std::ostream &nullstream()
{
    static std::ofstream os;
    if (!os.is_open())
        os.open("/dev/null", std::ofstream::out | std::ofstream::app);
    return os;
}

// this flag determines whether to use output or not
#define OUTPUT_LOG
// #undef OUTPUT_LOG

inline std::ostream &log()
{
#ifdef OUTPUT_LOG
    static std::ostream &out = std::cerr;
#else
    static std::ostream &out = nullstream();
#endif
    return out << "[" << std::hex << std::this_thread::get_id() << "]" << std::dec;
}

// safely construct a c++ string from a const char pointer
// according to cppreference constructing a c++ string from a nullptr
// is undefined behavior
inline std::string safe_construct_string(const char *cstr)
{
    return cstr == nullptr ? std::string() : std::string(cstr);
}

#endif