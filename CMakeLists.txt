cmake_minimum_required(VERSION 3.10)
project(diskhash VERSION 1.0 LANGUAGES CXX)

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

# Link -lrt on Linux (used by tests for clock_gettime)
if(UNIX AND NOT APPLE)
    target_link_libraries(diskhash PUBLIC rt)
endif()

# Test executable
enable_testing()
add_executable(test4
    tests/test4.cpp
    tests/test_catalogue.cpp
    tests/test_container.cpp
    tests/test_file_map.cpp
    tests/test_hash_map.cpp
    tests/test_vbe.cpp
)
target_link_libraries(test4 PRIVATE diskhash)
add_test(NAME test4 COMMAND test4)
