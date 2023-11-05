cmake_minimum_required(VERSION 3.10)

project(SlorpLanguage)

# Add source files
add_executable(Slorp
    src/main.c
    src/chunk.c
    src/memory.c
    src/debug.c
    src/value.c
    src/vm.c
)

# Include directories!
target_include_directories(Slorp PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

# Enable warnings
if(CMAKE_COMPILER_IS_GNUCC)
    message(STATUS "GNU C Compiler detected, adding compile flags")
    target_compile_options(Slorp PRIVATE -Wall -Wextra)
endif(CMAKE_COMPILER_IS_GNUCC)
