cmake_minimum_required(VERSION 3.0.0)
project(gstreamer-helloworld VERSION 0.1.0)

include(CTest)
enable_testing()

# set C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# use PkgConfig to configure gstreamer & azure storage cpp sdk
find_package(PkgConfig)
# use pkg-config to get Gstreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(LIBCURL REQUIRED libcurl)
pkg_check_modules(OPENSSL REQUIRED openssl)

# include header files
include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${LIBCURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
)

#include link directories
link_libraries(
    ${GLIB_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    /usr/local/lib/libazure-storage-lite.so
    ${OPENSSL_LIBRARIES}
    ${LIBCURL_LIBRARIES}
)

add_executable(gstreamer-helloworld main.cpp utils.cpp utils.h)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)