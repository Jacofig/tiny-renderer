cmake_minimum_required(VERSION 4.0)
project(rend)

set(CMAKE_CXX_STANDARD 20)

add_executable(rend main.cpp
        tgaimage.cpp
        tgaimage.h
        model.h
        model.cpp
        gl.h
        gl.cpp
        config.h)
