cmake_minimum_required(VERSION 3.10)

project(streak)

add_library(streak 
    "src/entry_point.cpp"
)

target_include_directories(streak PUBLIC "include")