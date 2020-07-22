cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin)

add_subdirectory(utils)

# specify compile targets
file(GLOB SOURCE_FILES *.c *.cpp *.h *.hpp)
add_library(gstazureelements SHARED ${SOURCE_FILES})
target_link_libraries(gstazureelements base64)