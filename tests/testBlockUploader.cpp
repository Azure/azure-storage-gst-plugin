#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "blockazureuploader.hpp"
#include "testUtils.hpp"

using namespace std::chrono_literals;
int main(int argc, char **argv)
{
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = false;

  // test block uploader
  gst::azure::storage::BlockAzureUploader uploader(
      account_name, account_key,
      4 * 1024 * 1024, 4, 16, 60000, use_https, std::string());
  auto loc = uploader.init("videostore", "teststream");
  // now upload a lot of contents...
  std::srand(std::time(nullptr));
  const char *bufs[4] = {
      getRandomBytes(1048576),
      getRandomBytes(1048576),
      getRandomBytes(1048576),
      getRandomBytes(1048576)};
  // upload 64MiB of content concurrently
  for (int i = 0; i < 64; i++)
    uploader.upload(loc, bufs[std::rand() % 4], 1048576);
  uploader.flush(loc);
  uploader.destroy(loc);
  return 0;
}