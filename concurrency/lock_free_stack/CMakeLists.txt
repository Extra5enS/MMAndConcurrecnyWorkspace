include_directories(include)

# Testing
add_gtest(
    NAME lock_free_stack_tests
    SOURCES tests/lock_free_stack_tests.cpp
)