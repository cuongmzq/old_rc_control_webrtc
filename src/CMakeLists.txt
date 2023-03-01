cmake_minimum_required(VERSION 2.8)

project(video LANGUAGES C)

set(CMAKE_CFLAGS "${CMAKE_CFLAGS} -lmmal -lmmal_core -lmmal_util -L/opt/vc/bin -lpthread -lvcos")
find_library(LIB
    NAMES mmal mmal_core mmal_util vcos pthread
    PATHS /opt/vc/lib/)

add_library(video OBJECT ./video.c)
link_libraries(${LIB})

include_directories(.)
include_directories(./include)
include_directories(./libs)
