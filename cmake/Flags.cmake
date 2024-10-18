set(CMAKE_CXX_FLAGS_DEBUG "\
-pthread -g -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ \
-fsanitize=address,undefined,leak,shift,float-divide-by-zero,signed-integer-overflow\
")
set(CMAKE_CXX_FLAGS_RELEASE "-pthread -O3 -g -DNDEBUG")