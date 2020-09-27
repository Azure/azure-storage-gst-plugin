#ifndef _AZURE_ELEMENTS_UPLOADER_COMMON_HPP_
#define _AZURE_ELEMENTS_UPLOADER_COMMON_HPP_

#include <string>
#include <utility>

namespace gst {
namespace azure {
namespace storage {

typedef std::pair<std::string, std::string> AzureLocation;
typedef std::pair<const char *, size_t> UploadBuffer;
typedef long long offset_t;
typedef unsigned long long size_t;

}
}
}

#endif