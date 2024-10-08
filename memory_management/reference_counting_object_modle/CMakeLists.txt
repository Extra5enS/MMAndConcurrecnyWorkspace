include_directories(include)

# Testing
add_gtest(
    NAME reference_counting_object_modle
    SOURCES tests/gc_test.cpp tests/delete_detector.cpp
)