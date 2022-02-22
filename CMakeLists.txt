cmake_minimum_required(VERSION 3.16)
project(libnet C)
set(CMAKE_INSTALL_PREFIX /usr)

include_directories("./include" "./libs/libmc/include")

SET(CMAKE_C_FLAGS "-std=c89 -gdwarf-4 -g3")

add_subdirectory(src)
