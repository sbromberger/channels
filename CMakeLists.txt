cmake_minimum_required(VERSION 3.22)
project(testchannels)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
		cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  concurrentqueue
  URL https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.4.tar.gz
)
FetchContent_MakeAvailable(concurrentqueue)
include_directories(include/)

add_executable(testchannels src/testchannels.cpp)
target_link_libraries(testchannels PRIVATE concurrentqueue)
# target_link_libraries(godbolt PRIVATE cpp_httplib)
