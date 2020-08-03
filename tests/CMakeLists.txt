cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Debug)

# gstreamer check dependency

find_package(PkgConfig)
pkg_check_modules(GSTREAMER_CHECK REQUIRED gstreamer-check-1.0)

include_directories(${GSTREAMER_CHECK_INCLUDE_DIRS})
link_libraries(
    ${GSTREAMER_CHECK_LIBRARIES}
    gstazureelements
)

add_executable(simple-uploader-test testSimpleUploader.cpp)
add_executable(block-uploader-test testBlockUploader.cpp)
add_executable(downloader-test testBlockDownloader.cpp)
add_executable(azure-sink-test azuresink.c)

add_test(
    NAME TestSimpleUploader
    COMMAND simple-uploader-test 
)

add_test(
    NAME TestBlockUploader
    COMMAND block-uploader-test
)

add_test(
    NAME TestDownloader
    COMMAND downloader-test
)

add_test(
    NAME TestAzureSink
    COMMAND azure-sink-test
)
