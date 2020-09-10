#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "simpleazureuploader.hpp"
#include "testUtils.hpp"

using namespace std::chrono_literals;
int main(int argc, char **argv)
{
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = true;

  // Test our azure uploader
  gst::azure::storage::SimpleAzureUploader uploader(account_name, account_key, use_https, std::string());
  uploader.init("videostore", "teststream.txt");
  // upload 32MiB of random stream, size of each trunk is random
  unsigned sz = 32 * 1024 * 1024;
  for (unsigned i = 0; i < sz;)
  {
    unsigned cur_size = std::rand();
    cur_size = std::min(cur_size, sz - i);
    const char *buf = getRandomBytes(cur_size);
    uploader.upload(buf, cur_size);
    i += cur_size;
    delete[] buf;
  }
  uploader.flush();
  uploader.destroy();
  return 0;
}
