cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin VERSION 0.1.0)

# specify compile targets
include_directories(util)
file(GLOB SOURCE_FILES util/* *.c *.cpp *.h *.hpp)
add_library(gstazureelements SHARED ${SOURCE_FILES})
