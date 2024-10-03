include_directories(include)

# Testing
add_gtest(
    NAME reference_counting_gc
    SOURCES tests/gc_test.cpp tests/delete_detector.cpp
)
