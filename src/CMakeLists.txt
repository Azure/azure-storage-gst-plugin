cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin)

# specify compile targets
file(GLOB SOURCE_FILES *.c *.cpp)
set(SOURCE_FILES
    gstazureelements.c
    gstazuresink.c
    gstazuresrc.c
    azuredownloader.cpp
    blockazureuploader.cpp
    simpleazureuploader.cpp
    
    utils/base64.cpp
)

add_library(gstazureelements SHARED ${SOURCE_FILES})
target_link_libraries(gstazureelements)