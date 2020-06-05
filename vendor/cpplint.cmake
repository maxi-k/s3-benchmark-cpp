# ---------------------------------------------------------------------------
# S3 Benchmark - Linter
# ---------------------------------------------------------------------------

include(ExternalProject)
find_package(PythonInterp)

# Get cpplint
ExternalProject_Add(
    cpplint_src
    PREFIX "vendor/cpplint"
    GIT_REPOSITORY "https://github.com/google/styleguide.git"
    GIT_TAG 2282a0495b5a1f8b6c9fa5c6695be7b5377e805e
    TIMEOUT 10
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)

# CppLint script path
ExternalProject_Get_Property(cpplint_src source_dir)
set(CPPLINT_SCRIPT_PATH "${source_dir}/cpplint/cpplint.py")

# Add target to lint the given source files
function(add_cpplint_target TARGET INPUT)
    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "CppLint requires a python interpreter")
    endif()
    if(NOT INPUT)
        add_custom_target(${TARGET})
        return()
    endif()

    # Remove duplicates & sort
    list(REMOVE_DUPLICATES INPUT)
    list(SORT INPUT)

    # Add target
    add_custom_target(${TARGET}
        COMMAND ${CMAKE_COMMAND} -E chdir
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${PYTHON_EXECUTABLE}
                ${CPPLINT_SCRIPT_PATH}
                "--counting=detailed"
                "--extensions=cc,h,cpp,hpp"
                "--linelength=200"
                "--filter=-legal/copyright,-runtime/references,-readability/todo"
                ${INPUT}
        DEPENDS cpplint_src
        COMMENT "Running ${TARGET}"
        VERBATIM)
endfunction()
