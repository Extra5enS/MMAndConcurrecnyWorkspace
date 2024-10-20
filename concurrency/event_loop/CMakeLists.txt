include_directories(include)

# Testing
add_gtest(
    NAME event_loop_tests
    SOURCES tests/event_loop_tests.cpp
)