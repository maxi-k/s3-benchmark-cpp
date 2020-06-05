# ---------------------------------------------------------------------------
# S3 Benchmark - Library
# ---------------------------------------------------------------------------

include("${BENCH_LIB_DIR}/include/local.cmake")
include_directories(
    ${BENCH_LIB_DIR}/include
    ${AWS_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
    ${GFLAGS_INCLUDE_DIR}
)

include("${BENCH_LIB_DIR}/src/local.cmake")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# Locate the AWS SDK for C++ package.
# defines AWSSDK_LINK_LIBRARIES
find_package(AWSSDK REQUIRED COMPONENTS s3)

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

add_library(s3benchmark_lib STATIC ${BENCH_LIB_SRC_CC})
target_link_libraries(s3benchmark_lib ${AWSSDK_LINK_LIBRARIES})
