#ifndef _AZURE_SINK_BASE64_HPP_
#define _AZURE_SINK_BASE64_HPP_

#include <sstream>

std::string base64_encode(const char *bytes_to_encode, unsigned int len);
std::string base64_encode(const std::string s);

#endif