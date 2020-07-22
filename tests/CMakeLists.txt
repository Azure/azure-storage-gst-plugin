cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin VERSION 0.1.0)

include_directories(${AZURE_SINK_HEADERS_DIR})
link_libraries(gstazureelements)
add_executable(simple-uploader-test testSimpleUploader.cpp)
add_executable(block-uploader-test testBlockUploader.cpp)