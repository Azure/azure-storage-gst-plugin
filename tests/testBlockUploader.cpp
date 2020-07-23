#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "blockazureuploader.hpp"

using namespace std::chrono_literals;
int main(int argc, char** argv) {
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = true;

  // Test our azure uploader
  gst::azure::storage::BlockAzureUploader uploader(account_name.c_str(), account_key.c_str(), use_https);
  auto loc = uploader.init("videostore", "teststream");
  uploader.upload(loc, "asdfasdfasdf12341234", 20);
  uploader.upload(loc, "hahaha1234", 10);
  std::this_thread::sleep_for(2s);
  uploader.upload(loc, "heiheihei", 9);
  uploader.flush(loc);
  uploader.destroy(loc);
  return 0;
} 