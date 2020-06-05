# ---------------------------------------------------------------------------
# S3 Benchmark
# ---------------------------------------------------------------------------

project(S3Benchmark LANGUAGES C CXX ASM)
cmake_minimum_required(VERSION 3.8.2)

# Use shared libraries, which is the default for the AWS C++ SDK build.
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

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
# include("${CMAKE_SOURCE_DIR}/vendor/awssdk.cmake")

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

message(STATUS "[S3 BENCHMARK] settings")
message(STATUS "    BENCH_LIB_DIR               = ${BENCH_LIB_DIR}")
message(STATUS "    BENCH_CLI_DIR               = ${BENCH_CLI_DIR}")
message(STATUS "[CMAKE] settings")
message(STATUS "    CMAKE_SOURCE_DIR            = ${CMAKE_SOURCE_DIR}")
message(STATUS "    CMAKE_SHARED_LIBRARY_SUFFIX = ${CMAKE_SHARED_LIBRARY_SUFFIX}")
message(STATUS "    CMAKE_SHARED_LIBRARY_PREFIX = ${CMAKE_SHARED_LIBRARY_PREFIX}")
message(STATUS "[AWSSDK] settings")
message(STATUS "    AWS_INCLUDE_DIRS            = ${AWS_INCLUDE_DIRS}")
message(STATUS "    AWSSDK_LINK_LIBRARIES       = ${AWSSDK_LINK_LIBRARIES}")
message(STATUS "[CPPLINT] settings")
message(STATUS "    CPPLINT_SCRIPT_PATH         = ${CPPLINT_SCRIPT_PATH}")
message(STATUS "    lint_targets                = ${CMAKE_CXX_CPPLINT}")
message(STATUS "[TEST] settings")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
message(STATUS "    GMOCK_INCLUDE_DIR           = ${GMOCK_INCLUDE_DIR}")
message(STATUS "    GMOCK_LIBRARY_PATH          = ${GMOCK_LIBRARY_PATH}")
