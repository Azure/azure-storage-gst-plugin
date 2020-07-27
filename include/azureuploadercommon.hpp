#ifndef _AZURE_ELEMENTS_UPLOADER_COMMON_HPP_
#define _AZURE_ELEMENTS_UPLOADER_COMMON_HPP_

#include <string>
#include <utility>

namespace gst {
namespace azure {
namespace storage {

typedef std::pair<std::string, std::string> AzureUploadLocation;
typedef std::pair<const char *, size_t> UploadBuffer;

}
}
}

#endif