cmake_minimum_required(VERSION 3.15)
project(bloomfilter C)

set(CMAKE_C_STANDARD 99)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wall -g -std=c99")

set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")

include_directories(${CMAKE_CURRENT_LIST_DIR}/murmurhash3)

#add_executable(bloomfilter main.c bloomfilter/bloomfilter.h bloomfilter/bloomfilter.c murmurhash3/murmurhash3.c murmurhash3/murmurhash3.h)
add_library(bloomfilter SHARED main.c bloomfilter/bloomfilter.h bloomfilter/bloomfilter.c murmurhash3/murmurhash3.c murmurhash3/murmurhash3.h)

TARGET_LINK_LIBRARIES(bloomfilter m)
