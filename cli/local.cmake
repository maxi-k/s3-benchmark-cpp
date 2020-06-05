# ---------------------------------------------------------------------------
# S3 Benchmark - CLI tool
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Include AWS Libraries
# ---------------------------------------------------------------------------

#### TODO ####

# ---------------------------------------------------------------------------
# Include Project Directories
# ---------------------------------------------------------------------------
include("${BENCH_CLI_DIR}/include/local.cmake")
include_directories(
    ${CMAKE_LIBRARY_DIR}/include
    ${GTEST_INCLUDE_DIR}
    ${GMOCK_INCLUDE_DIR}
    ${GFLAGS_INCLUDE_DIR}
)

include("${BENCH_CLI_DIR}/src/local.cmake")

add_executable(s3benchmark_cli_exec ${BENCH_CLI_SRC_CC})
target_link_libraries(s3benchmark_cli_exec
                        # ${BENCH_CLI_AWS_SDK_LIB}
                        # ${RASPBPI_PCSC_LIB}
                        )
