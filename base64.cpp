#include "base64.hpp"

#include <string>
#include <sstream>
#include <cctype>

static constexpr char b64chars[] = 
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

std::string base64_encode(const char *bytes_to_encode, unsigned int len)
{
  char buf[4] = {0};
  const char *pos = bytes_to_encode;
  std::ostringstream oss;
  while(len > 3) {
    buf[0] = b64chars[(pos[0] & 0xfc) >> 2];
    buf[1] = b64chars[(pos[0] & 0x03) << 4 + (pos[1] & 0xf0) >> 4];
    buf[2] = b64chars[(pos[1] & 0x0f) << 2 + (pos[2] & 0xc0) >> 6];
    buf[3] = b64chars[pos[2] & 0x3f];
    oss << buf; // there should not be any '\0'
    len -= 3;
    pos += 3;
  }
  switch(len)
  {
    case 2:
      buf[0] = b64chars[(pos[0] & 0xfc) >> 2];
      buf[1] = b64chars[(pos[0] & 0x03) << 4 + (pos[1] & 0xf0) >> 4];
      buf[2] = b64chars[(pos[1] & 0x0f) << 2];
      buf[3] = '=';
      oss << buf;
      break;
    case 1:
      buf[0] = b64chars[(pos[0] & 0xfc) >> 2];
      buf[1] = b64chars[(pos[0] & 0x03) << 4];
      buf[2] = '=';
      buf[3] = '=';
      oss << buf;
      break;
    default:
      // do nothing
      break;
  }
  return oss.str();
}