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
int main(int argc, char **argv)
{
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "***";
  bool use_https = true;

  // use default blob_endpoint(which is empty)
  AzureDownloader downloader(account_name, account_key,
                             4, 2 * one_mb, 4, use_https, std::string());
  downloader.init("videostore", "costa_rica.webm");
  char *buffer = new char[4 * one_mb];

  // download first 32MiB from it, 512KiB ~ 4MiB each time
  std::ofstream s("costa_rica_first32.webm", std::ios::out | std::ios::binary | std::ios::trunc);
  std::srand(std::time(nullptr));

  for (unsigned long i = 0; i < 32 * one_mb;)
  {
    unsigned cur = std::rand() % (4 * one_mb - one_mb / 2) + one_mb / 2;
    cur = std::min((unsigned long long)cur, 32 * one_mb - i);
    log() << "!!!Requesting to read " << cur / 1024 << "KiB." << std::endl;
    downloader.read(buffer, cur);
    s.write(buffer, cur);
    i += cur;
  }
  s.close();
  downloader.destroy();
  log() << "Finished reading." << std::endl;
}