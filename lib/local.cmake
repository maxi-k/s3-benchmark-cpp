# ---------------------------------------------------------------------------
# S3 Benchmark - Library
# ---------------------------------------------------------------------------

include("${BENCH_LIB_DIR}/include/local.cmake")
include_directories(
    ${BENCH_LIB_DIR}/include
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
    ${GFLAGS_INCLUDE_DIR}
)

include("${BENCH_LIB_DIR}/src/local.cmake")
