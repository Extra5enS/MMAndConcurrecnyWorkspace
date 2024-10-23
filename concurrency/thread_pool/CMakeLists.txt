include_directories(include)

# Testing
add_gtest(
    NAME thread_pool_tests
    SOURCES tests/thread_pool_tests.cpp
)