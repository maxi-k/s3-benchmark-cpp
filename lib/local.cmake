# ---------------------------------------------------------------------------
# Benchmark - Library
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

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

# ---------------------------------------------------------------------------
# Libraries
# ---------------------------------------------------------------------------

add_library(s3benchmark_lib STATIC ${BENCH_LIB_SRC_CC})
target_link_libraries(s3benchmark_lib ${AWS_LINK_LIBRARIES} ${CURL_LIBRARY} Threads::Threads)

# Link against CoreFoundation if on mac
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_library(CORE_FOUNDATION CoreFoundation)
    target_link_libraries(s3benchmark_lib ${CORE_FOUNDATION})
endif()
