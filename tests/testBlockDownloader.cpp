#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "azuredownloader.hpp"

#include "testUtils.hpp"
#include "utils/common.hpp"

using namespace std::chrono_literals;
using namespace gst::azure::storage;

constexpr gst::azure::storage::size_t one_mb = 1 * 1024 * 1024;
int main(int argc, char** argv) {
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = true;
  
  AzureDownloader downloader(account_name.c_str(), account_key.c_str(),
    use_https, 4, one_mb, one_mb);
  downloader.init("videostore", "costa_rica.webm");
  char* buffer = new char[one_mb];
  downloader.read(buffer, one_mb);

  log() << "Finished reading." << std::endl;
}