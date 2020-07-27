#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "storage_account.h"
#include "storage_credential.h"
#include "get_blob_request_base.h"
#include "blob/blob_client.h"

#include "testUtils.hpp"
#include "utils/common.hpp"

using namespace std::chrono_literals;
using namespace ::azure::storage_lite;
int main(int argc, char** argv) {
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = true;

  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(account_name, account_key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(account_name, credential, use_https);
  auto client = blob_client(storage_account, 4);
  std::cout << "Client created." << std::endl;
  // read content
  std::stringstream ss;
  auto fut = client.download_blob_to_stream("videostore", "costa_rica.webm", 0, 1L*1024L*1024L*1024L, ss);
  std::string content;
  std::cout << "Request sent" << std::endl;
  while(ss >> content)
  {
    std::cout << "Reading, content length = %d" << content.size() << std::endl;
  }
  auto result = fut.get();
  handle(result);
  return 0;
}