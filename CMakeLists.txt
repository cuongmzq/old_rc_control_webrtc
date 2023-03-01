cmake_minimum_required(VERSION 3.3)
project(ROBAK_VEHICLE_CONTROL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lmmal -lmmal_core -lmmal_util -L/opt/vc/lib -lpthread -lvcos")
set(CMAKE_CFLAGS "${CMAKE_CFLAGS} -L/opt/vc/lib -lpthread -lvcos")

set(SRC_PATH ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_PATH ${SRC_PATH}/include)
set(LIBS_PATH ${SRC_PATH}/libs)
set(LIB_SEASOCKS_PATH ${LIBS_PATH}/libseasocks.so)
set(LIB_WEBRTC_DATA_CHANNEL ${LIBS_PATH}/libdatachannel.so.0.17.9)

link_directories(/opt/vc/lib)

#set_source_files_properties(video.c DIRECTORY ${SRC_PATH} PROPERTIES LANGUAGE C)
add_executable(ROBAK_VEHICLE_CONTROL ${SRC_PATH}/server.cpp)

#add_subdirectory(${SRC_PATH})
#add_executable(MMALCAM ${SRC_PATH}/mmalcam/mmalcam.c)
#find_library(LIB
#    NAMES mmal mmal_core mmal_util vcos pthread
#    PATHS /opt/vc/lib/)
target_link_libraries(ROBAK_VEHICLE_CONTROL ${LIB_SEASOCKS_PATH} ${LIB_WEBRTC_DATA_CHANNEL} pigpio)
#target_link_libraries(MMALCAM ${LIB})

#include_directories(${SRC_PATH})
include_directories(${LIBS_PATH})
include_directories(${INCLUDE_PATH})
include_directories(${INCLUDE_PATH}/interface)
include_directories(/opt/vc/include/)
add_custom_command(
    TARGET ROBAK_VEHICLE_CONTROL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ROBAK_VEHICLE_CONTROL
            ${PROJECT_SOURCE_DIR}/robak)
