set(CMAKE_CXX_FLAGS_DEBUG "\
-g -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function \
-fsanitize=address,undefined,leak,integer,shift,float-divide-by-zero,\
implicit-conversion,signed-integer-overflow\
")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -DNDEBUG")
