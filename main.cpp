#include <iostream>
#include <memory>

// #include <gstreamer-1.0/gst/gst.h>
#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/put_block_list_request.h"
#include "blob/put_block_request.h"

#include "utils.h"

using std::make_shared;

int main(int argc, char** argv) {
    // settings
    std::string account_name = "gstvideostore";
    std::string account_key = "vIjLNYmW60yVH1nLc08u/KGJNXZl6Gzy7zdGtpSK3JJ6vSQfjRQUB8z/UpEoS27J4Tkl1/a30blvTPurkdC3jA==";
    bool use_https = true;
    std::string blob_endpoint = "";
    int connection_count = 2;

    // Setup the client
    auto credential = make_shared<azure::storage_lite::shared_key_credential>(account_name, account_key);
    auto storage_account = make_shared<azure::storage_lite::storage_account>(account_name, credential, use_https, blob_endpoint);
    azure::storage_lite::blob_client client(storage_account, connection_count);
    // Start using
    auto result = client.append_block_from_stream("videostore", "teststream", std::cin);
    result.wait();
    auto r = result.get();
    handle(r);
    return 0;
}
