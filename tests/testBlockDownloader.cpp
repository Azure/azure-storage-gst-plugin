#include <sstream>
#include <iostream>
#include <chrono>
#include <fstream>
#include <thread>
#include <cstdlib>
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
    use_https, 4, 2 * one_mb, 4);
  downloader.init("videostore", "costa_rica.webm");
  char* buffer = new char[4 * one_mb];
  
  // download first 128MiB from it, 512KiB ~ 4MiB each time
  std::ofstream s("costa_rica_first640.webm", std::ios::out | std::ios::binary | std::ios::trunc);
  std::srand(std::time(nullptr));

  for (unsigned long i = 0; i < 640 * one_mb;)
  {
    unsigned cur = std::rand() % (4 * one_mb - one_mb / 2) + one_mb / 2;
    cur = std::min((unsigned long long)cur, 640 * one_mb - i);
    log() << "!!!Requesting to read " << cur / 1024 << "KiB." << std::endl;
    downloader.read(buffer, cur);
    s.write(buffer, cur);
    i += cur;
  }
  s.close();
  downloader.destroy();
  log() << "Finished reading." << std::endl;
}