include_directories(include)
include_directories("${PROJECT_ROOT}/libs/mlib/Utils/include")

# Testing
add_gtest(
    NAME reference_counting_gc
    SOURCES tests/gc_test.cpp tests/delete_detector.cpp
)
