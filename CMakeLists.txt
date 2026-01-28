cmake_minimum_required(VERSION 3.15)
project(diskhash VERSION 1.0 LANGUAGES CXX)

# Default to Release build for optimizations (-O3 -DNDEBUG)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Library target: libdiskhash
# Platform-specific source selection:
#   Linux:   src/linux/file_map.cpp
#   Windows: src/windows/file_map.cpp
add_library(diskhash
    src/catalogue.cpp
    src/container.cpp
    $<$<PLATFORM_ID:Linux>:src/linux/file_map.cpp>
    $<$<PLATFORM_ID:Windows>:src/windows/file_map.cpp>
)
target_include_directories(diskhash PUBLIC src)
set_target_properties(diskhash PROPERTIES POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Link -lrt on Linux (used by tests for clock_gettime)
if(UNIX AND NOT APPLE)
    target_link_libraries(diskhash PUBLIC rt)
endif()

# Test executable using Boost.Test (development only, not built for pip package)
if(NOT SKBUILD)
    enable_testing()
    find_package(Boost REQUIRED COMPONENTS unit_test_framework)

    add_executable(test4
        tests/test4.cpp
        tests/test_catalogue.cpp
        tests/test_container.cpp
        tests/test_file_map.cpp
        tests/test_hash_map.cpp
        tests/test_vbe.cpp
    )
    target_link_libraries(test4 PRIVATE diskhash Boost::unit_test_framework)
    target_compile_definitions(test4 PRIVATE BOOST_TEST_DYN_LINK)
    add_test(NAME test4 COMMAND test4)
endif()

# HTTP Server executable
find_package(Boost REQUIRED COMPONENTS system program_options)

add_executable(diskhash_server
    src/server/main.cpp
    src/server/http_server.cpp
)
target_link_libraries(diskhash_server PRIVATE
    diskhash
    Boost::system
    Boost::program_options
    pthread
)
target_include_directories(diskhash_server PRIVATE src)
target_compile_features(diskhash_server PRIVATE cxx_std_17)

# Python bindings (built via scikit-build-core: pip install .)
if(SKBUILD)
    find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)
    find_package(nanobind CONFIG REQUIRED)
    nanobind_add_module(_diskhash src/bindings.cpp)
    target_link_libraries(_diskhash PRIVATE diskhash)
    install(TARGETS _diskhash LIBRARY DESTINATION diskhash)
    install(TARGETS diskhash_server RUNTIME DESTINATION ${SKBUILD_SCRIPTS_DIR})
endif()
