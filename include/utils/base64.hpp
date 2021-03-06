#ifndef _AZURE_ELEMENTS_BASE64_HPP_
#define _AZURE_ELEMENTS_BASE64_HPP_

#include <sstream>

std::string base64_encode(const char *bytes_to_encode, unsigned int len);
std::string base64_encode(const std::string s);
std::string base64_encode(long long int i);

#endif