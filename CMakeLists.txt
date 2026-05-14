# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(usb)


set(C_OUT_PATH "${CMAKE_CURRENT_LIST_DIR}/main/proto")

set(PROTO_SRCS "${CMAKE_CURRENT_LIST_DIR}/proto/smak.proto")


set(NANO_PB_EXE_PATH "${CMAKE_CURRENT_LIST_DIR}/nanopb/generator/nanopb_generator.py")

add_custom_target(smak_proto
    COMMAND  python ${NANO_PB_EXE_PATH} -C -D ${C_OUT_PATH} ${PROTO_SRCS}
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
