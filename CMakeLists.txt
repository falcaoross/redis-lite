cmake_minimum_required(VERSION 3.10)
project(redis_lite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(redis-lite
    src/main.cpp
    src/cache.cpp
)

target_include_directories(redis-lite PRIVATE include)

# Optional: build type defaults
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
