cmake_minimum_required(VERSION 3.13.0)
project(azure-storage-gst-plugin VERSION 0.1.0)

# update submodules
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT)
        if(NOT GIT_SUBMODULE_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update failed with ${GIT_SUBMODULE_RESULT}. Please handle this manually.")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/extern/azure-storage-cpplite/CMakeLists.txt")
    message(FATAL_ERROR "Could not found submodule azure-storage-cpplite. Please turn on GIT_SUBMODULE or download the submodule manually.")
endif()

# set C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wall)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# use PkgConfig to configure gstreamer dependencies
find_package(PkgConfig)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_BASE REQUIRED gstreamer-base-1.0)

# find azure-storage-cpplite
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/azure-storage-cpplite)

# include header files
include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_BASE_INCLUDE_DIRS}
)
include_directories(${PROJECT_SOURCE_DIR}/include)

#include link directories
link_libraries(
    azure-storage-lite
    ${GLIB_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_BASE_LIBRARIES}
    pthread
)

add_subdirectory(src)

# tests
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()

# installing
install(TARGETS gstazureelements
    LIBRARY DESTINATION lib/gstreamer-1.0)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
