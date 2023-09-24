cmake_minimum_required(VERSION 3.1...3.27)

project(
  Glimmer
  VERSION 1.0
  LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

FILE(GLOB SRCS_GLIMMER_CORE
   src/core/commandqueue.h
   src/core/commandqueue.cpp
   src/core/d3dx12.h
   src/core/game.h
   src/core/game.cpp
   src/core/headers.h
   src/core/events.h
   src/core/window.h
   src/core/window.cpp
   src/core/application.h
   src/core/application.cpp
)


set(SRCS_MAIN  
   src/main.cpp
)

add_executable(Glimmer ${SRCS_MAIN} ${SRCS_GLIMMER_CORE})

source_group("source" FILES ${SRCS_MAIN})
source_group("source\\core" FILES ${SRCS_GLIMMER_CORE})

target_include_directories(Glimmer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/core
)

