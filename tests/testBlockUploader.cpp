#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "blockazureuploader.hpp"
#include "testUtils.hpp"

using namespace std::chrono_literals;
int main(int argc, char** argv) {
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = false;

  // Test our azure uploader
  gst::azure::storage::BlockAzureUploader uploader(account_name.c_str(), account_key.c_str(), use_https);
  auto loc = uploader.init("videostore", "teststream");
  // now upload a lot of contents...
  std::srand(std::time(nullptr));
  const char *bufs[4] = {
    getRandomBytes(1048576),
    getRandomBytes(1048576),
    getRandomBytes(1048576),
    getRandomBytes(1048576)
  };
  // upload 128MiB of content
  for(int i = 0; i < 128; i++)
    uploader.upload(loc, bufs[std::rand() % 4], 1048576);
  uploader.flush(loc);
  uploader.destroy(loc);
  return 0;
}