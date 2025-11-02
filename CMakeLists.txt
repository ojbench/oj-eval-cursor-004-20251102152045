cmake_minimum_required(VERSION 3.10)
project(BookstoreManagementSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output binary must be named 'code'
add_executable(code src/main.cpp)

# Optimize for speed, but keep debug symbols locally
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(code PRIVATE -O2 -pipe -Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion)
endif()
