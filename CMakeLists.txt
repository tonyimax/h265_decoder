cmake_minimum_required(VERSION 3.31)
set(PROJ_NAME h265_decoder)
project(${PROJ_NAME})

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWSCALE REQUIRED libswscale)
pkg_check_modules(SDL2 REQUIRED sdl2)

message( ${AVCODEC_INCLUDE_DIRS})
message( ${AVFORMAT_INCLUDE_DIRS})
message( ${AVUTIL_INCLUDE_DIRS})
message( ${SWSCALE_INCLUDE_DIRS})
message( ${SDL2_INCLUDE_DIRS})

message( ${AVCODEC_LIBRARIES})
message( ${AVFORMAT_LIBRARIES})
message( ${AVUTIL_LIBRARIES})
message( ${SWSCALE_LIBRARIES})
message( ${SDL2_LIBRARIES})


add_executable(${PROJ_NAME} main.cpp)

target_include_directories(
        ${PROJ_NAME} PRIVATE
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
        ${SWSCALE_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(
        ${PROJ_NAME}
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        ${SDL2_LIBRARIES}
        x265
)
