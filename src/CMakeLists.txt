cmake_minimum_required (VERSION 3.5.1)

project (ntttcp)

set (SRC_NTTTCP
endpointsync.c
logger.c
main.c
multithreading.c
ntttcp.c
oscounter.c
parameter.c
tcpstream.c
throughputmanagement.c
udpstream.c
util.c)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -O2 -fno-strict-aliasing")

add_executable(${PROJECT_NAME} ${SRC_NTTTCP})
target_link_libraries(${PROJECT_NAME} pthread)
