cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin)

# specify compile targets
add_library(base64 SHARED STATIC base64.cpp)