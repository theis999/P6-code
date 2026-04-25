cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(threads)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)

target_sources(app PRIVATE 
    src/main.c 
)
