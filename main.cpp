#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/put_block_list_request.h"
#include "blob/put_block_request.h"

#include "azureuploader.hpp"

#include "utils.h"

using namespace std::chrono_literals;
int main(int argc, char** argv) {
  // settings
  std::string account_name = "gstvideostore";
  std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
  bool use_https = true;

  // Test our azure uploader
  gst::azure::storage::AzureUploader uploader(account_name.c_str(), account_key.c_str(), use_https);
  auto loc = uploader.init("videostore", "teststream");
  uploader.upload(loc, "asdfasdfasdf", 12);
  uploader.upload(loc, "hahaha", 6);
  std::this_thread::sleep_for(2s);
  uploader.upload(loc, "heiheihei", 9);
  uploader.flush(loc);
  uploader.destroy(loc);
  return 0;
}
