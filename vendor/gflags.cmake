# ---------------------------------------------------------------------------
# S3 Benchmark - GFLAGS
# ---------------------------------------------------------------------------

ExternalProject_Add(gflags-src
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG v2.2.2
  PREFIX "vendor/gflags"
  INSTALL_DIR "vendor/gflags/install"
  CMAKE_ARGS
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_STATIC_LIBS=ON
        -DBUILD_gflags_LIB=OFF
        -DBUILD_gflags_nothreads_LIB=ON
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/gflags/install
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libgflags_nothreads.a
)

ExternalProject_Get_Property(gflags-src INSTALL_DIR)
message(STATUS "[GFLAGS] INSTALL_DIR = ${INSTALL_DIR}")

set(GFLAGS_INCLUDE_DIR  ${INSTALL_DIR}/include)
set(GFLAGS_LIBRARY_PATH ${INSTALL_DIR}/lib/libgflags_nothreads.a)
file(MAKE_DIRECTORY ${GFLAGS_INCLUDE_DIR})

add_library(gflags-static STATIC IMPORTED)

set_property(TARGET gflags-static PROPERTY IMPORTED_LOCATION ${GFLAGS_LIBRARY_PATH})
set_property(TARGET gflags-static APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GFLAGS_INCLUDE_DIR})

set(GFLAGS_LINK_LIBRARIES gflags-static)

add_dependencies(gflags-static gflags-src)
