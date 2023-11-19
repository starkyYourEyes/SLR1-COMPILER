cmake_minimum_required(VERSION 3.23)

# set the project name
project(p)

include_directories(.)

add_executable(${PROJECT_NAME}
        first_follow.h
        lexical.h
        main.cpp
        production.h
        slr1.h)
