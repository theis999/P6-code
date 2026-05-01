cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(threads)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)
include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)

zephyr_nanopb_sources(app src/smak.proto)

target_sources(app PRIVATE 
    src/main.c 
    src/model.c
    src/fsm.c
)
