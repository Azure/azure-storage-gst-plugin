cmake_minimum_required(VERSION 3.0.0)
project(azure-storage-gst-plugin VERSION 0.1.0)

include(CTest)
enable_testing()

# set C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set debug build type
set(CMAKE_BUILD_TYPE Debug)

# use PkgConfig to configure gstreamer & azure storage cpp sdk
find_package(PkgConfig)
# use pkg-config to get Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)
pkg_check_modules(LIBCURL REQUIRED libcurl)
pkg_check_modules(OPENSSL REQUIRED openssl)

# include header files
include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

#include link directories
link_libraries(
    ${GLIB_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    /usr/local/lib/libazure-storage-lite.so
    ${OPENSSL_LIBRARIcES}
    ${LIBCURL_LIBRARIES}
    pthread
)

# specify compile targets
file(GLOB SOURCE_FILES *.c *.cpp *.h *.hpp)
list(REMOVE_ITEM SOURCE_FILES main.cpp)
add_library(gstazurestorageplugin SHARED ${SOURCE_FILES})
add_executable(azure-storage-gst-plugin-test main.cpp)
target_link_libraries(azure-storage-gst-plugin-test gstazurestorageplugin)

add_compile_options(-Wall)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)