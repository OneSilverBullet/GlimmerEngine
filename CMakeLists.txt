cmake_minimum_required(VERSION 3.1...3.27)

project(
  Glimmer
  VERSION 1.0
  LANGUAGES CXX)


set(SOURCES
    include/headers.h
    include/mathlib.h
    apps/main.cpp
)

add_executable(Glimmer ${SOURCES})

target_include_directories(Glimmer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include/
)