cmake_minimum_required(VERSION 3.31)
project(h265_decoder)

set(CMAKE_CXX_STANDARD 20)

if (UNIX AND APPLE)
    include_directories( /usr/local/Cellar/ffmpeg/7.1.1_3/include )
    link_directories( /usr/local/Cellar/ffmpeg/7.1.1_3/lib)
endif ()

add_executable(h265_decoder main.cpp)

target_link_libraries(
        h265_decoder
        avcodec
        avformat
        avutil
        swscale
)
