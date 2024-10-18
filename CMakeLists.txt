
cmake_minimum_required(VERSION 3.10)
project(MMAndConcurrency)

enable_language(CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROJECT_BINARY_ROOT ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS -pthread)


# Cody style
include(cmake/ClangTidy.cmake)

# Testing framework 
include(cmake/TestFramework.cmake)

# include root for clear include path usage
include_directories(${PROJECT_ROOT})

# Testing 
add_subdirectory(${PROJECT_ROOT}/memory_management/bump_pointer_allocator)
add_subdirectory(${PROJECT_ROOT}/memory_management/run_of_slots_allocator)
add_subdirectory(${PROJECT_ROOT}/memory_management/free_list_allocator)
add_subdirectory(${PROJECT_ROOT}/memory_management/reference_counting_gc)
add_subdirectory(${PROJECT_ROOT}/memory_management/reference_counting_object_modle)

add_subdirectory(${PROJECT_ROOT}/concurrency/thread_safe_containers)
