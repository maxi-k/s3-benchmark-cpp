# ---------------------------------------------------------------------------
# Benchmark
# ---------------------------------------------------------------------------

project(Benchmark LANGUAGES C CXX ASM)
cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -Wall -Wextra -fno-omit-frame-pointer -march=native -Wno-unknown-pragmas -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wextra -fno-omit-frame-pointer -march=native -pthread")
# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------

set(BENCH_LIB_DIR "${CMAKE_SOURCE_DIR}/lib/")
set(BENCH_CLI_DIR "${CMAKE_SOURCE_DIR}/cli/")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/cpplint.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/awssdk.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/gflags.cmake")

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

include("${BENCH_LIB_DIR}/local.cmake")
if(${BENCH_TARGET} MATCHES "cli")
   include("${BENCH_CLI_DIR}/local.cmake")
else()
   message(WARN, "BENCH_TARGET not set, only building library")
endif()


include("${CMAKE_SOURCE_DIR}/test/local.cmake")

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

add_custom_target(lint)
add_dependencies(lint ${lint_targets})


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

message(STATUS "[BENCHMARK] settings")
message(STATUS "    BENCH_LIB_DIR               = ${BENCH_LIB_DIR}")
message(STATUS "    BENCH_CLI_DIR               = ${BENCH_CLI_DIR}")
message(STATUS "[CMAKE] settings")
message(STATUS "    CMAKE_SOURCE_DIR            = ${CMAKE_SOURCE_DIR}")
message(STATUS "    CMAKE_SHARED_LIBRARY_SUFFIX = ${CMAKE_SHARED_LIBRARY_SUFFIX}")
message(STATUS "    CMAKE_STATIC_LIBRARY_SUFFIX = ${CMAKE_STATIC_LIBRARY_SUFFIX}")
message(STATUS "    CMAKE_SHARED_LIBRARY_PREFIX = ${CMAKE_SHARED_LIBRARY_PREFIX}")
message(STATUS "    CMAKE_STATIC_LIBRARY_PREFIX = ${CMAKE_STATIC_LIBRARY_PREFIX}")
message(STATUS "[AWSSDK] settings")
message(STATUS "    AWS_INCLUDE_DIR             = ${AWS_INCLUDE_DIR}")
message(STATUS "    AWS_LINK_LIBRARIES          = ${AWS_LINK_LIBRARIES}")
message(STATUS "[CPPLINT] settings")
message(STATUS "    CPPLINT_SCRIPT_PATH         = ${CPPLINT_SCRIPT_PATH}")
message(STATUS "    lint_targets                = ${CMAKE_CXX_CPPLINT}")
message(STATUS "[TEST] settings")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
message(STATUS "    GMOCK_INCLUDE_DIR           = ${GMOCK_INCLUDE_DIR}")
message(STATUS "    GMOCK_LIBRARY_PATH          = ${GMOCK_LIBRARY_PATH}")
