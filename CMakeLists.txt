cmake_minimum_required(VERSION 3.16)

project(rdbms)

set(CMAKE_C_STANDARD 11)

add_executable(rdbms
    src/catalog.c
    src/executor.c
    src/index.c
    src/lexer.c
    src/main.c
    src/myparser.c
    src/mystorage.c
    src/utils.c
    src/web.c
)

target_include_directories(rdbms PRIVATE include)
