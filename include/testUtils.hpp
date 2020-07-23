#ifndef _AZURE_SINK_TEST_UTILS_HPP_
#define _AZURE_SINK_TEST_UTILS_HPP_

#include <sstream>
#include <random>

const char *getRandomBytes(unsigned int len)
{
    char *ret = new char[len];
    for(unsigned i = 0; i < len / sizeof(int) ; i++) {
        ((int *)ret)[i] = std::rand();
    }
    return ret;
}

#endif