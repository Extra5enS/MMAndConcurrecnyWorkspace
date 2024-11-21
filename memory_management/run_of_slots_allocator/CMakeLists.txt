include_directories(include)

# Testing
add_gtest(
    NAME run_of_slots_allocator
    SOURCES tests/allocator_test.cpp
)
