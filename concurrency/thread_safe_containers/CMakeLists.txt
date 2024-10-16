include_directories(include)

# Testing
add_gtest(
    NAME containers_tests
    SOURCES tests/containers_tests.cpp
)