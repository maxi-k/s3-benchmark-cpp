# ---------------------------------------------------------------------------
# S3 Benchmark - AWS SDK
# ---------------------------------------------------------------------------
# Modified from: http://fireflytech.org/2019/04/24/accessing-aws-s3-from-c-under-cmake/

Include(ExternalProject)

# Build a minimal AWS SDK for s3
ExternalProject_Add(awssdk
    PREFIX              ${CMAKE_BINARY_DIR}/vendor/awssdk
    GIT_REPOSITORY      https://github.com/aws/aws-sdk-cpp.git
    GIT_TAG             1.7.348
    UPDATE_COMMAND      ""
    CMAKE_ARGS
        -DBUILD_ONLY:STRING=s3
        -DBUILD_SHARED_LIBS=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        # -DCMAKE_SOURCE_DIR:PATH=<SOURCE_DIR>
        -DENABLE_TESTING:BOOL=OFF
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/awssdk
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
)

ExternalProject_Get_Property(awssdk INSTALL_DIR)
message(STATUS "[AWS] INSTALL_DIR = ${INSTALL_DIR}")

set(AWS_INCLUDE_DIR       ${INSTALL_DIR}/include)
set(AWS_LIBRARY_CORE_PATH ${INSTALL_DIR}/lib/libaws-cpp-sdk-core.so)
set(AWS_LIBRARY_S3_PATH   ${INSTALL_DIR}/lib/libaws-cpp-sdk-s3.so)
add_library(awssdk::core SHARED IMPORTED)
add_library(awssdk::s3   SHARED IMPORTED)

set_property(TARGET awssdk::core PROPERTY IMPORTED_LOCATION ${AWS_LIBRARY_CORE_PATH})
set_target_properties(awssdk::s3   PROPERTIES IMPORTED_LOCATION ${AWS_LIBRARY_S3_PATH})
set(AWS_LINK_LIBRARIES awssdk::s3 awssdk::core)
