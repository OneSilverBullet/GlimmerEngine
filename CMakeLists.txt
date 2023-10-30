cmake_minimum_required(VERSION 3.1...3.27)

project(
  Glimmer
  VERSION 1.0
  LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set( SHADER_FILES
    shaders/default_pixel.hlsl
    shaders/base_vertex.hlsl
)

set_source_files_properties( shaders/base_vertex.hlsl PROPERTIES 
    VS_SHADER_TYPE Vertex
    VS_SHADER_ENTRYPOINT VSMain
    VS_SHADER_MODEL 5.1
)

set_source_files_properties( shaders/default_pixel.hlsl PROPERTIES 
    VS_SHADER_TYPE Pixel
    VS_SHADER_ENTRYPOINT PSMain
    VS_SHADER_MODEL 5.1
)


FILE(GLOB SRCS_GLIMMER_CORE
   src/core/commandqueue.h
   src/core/commandqueue.cpp
   src/core/commandmanager.h
   src/core/commandmanager.cpp
   src/core/d3dx12.h
   src/core/game.h
   src/core/game.cpp
   src/core/headers.h
   src/core/events.h
   src/core/timer.h
   src/core/graphicscore.h
   src/core/graphicscore.cpp
   src/core/rootsignature.h
   src/core/rootsignature.cpp
   src/core/pso.h
   src/core/pso.cpp
   src/core/window.h
   src/core/window.cpp
   src/core/application.h
   src/core/application.cpp
   src/core/clientgame.h
   src/core/clientgame.cpp
   src/core/commandallocatorpool.h
   src/core/commandallocatorpool.cpp
)


set(SRCS_MAIN  
   src/main.cpp
)

add_executable(Glimmer WIN32 ${SRCS_MAIN} ${SRCS_GLIMMER_CORE} ${SHADER_FILES})

source_group("source" FILES ${SRCS_MAIN})
source_group("source\\core" FILES ${SRCS_GLIMMER_CORE})
source_group( "resources\\shaders" FILES ${SHADER_FILES} )

target_include_directories(Glimmer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/core
)

target_link_libraries(Glimmer 
   d3d12.lib 
   dxgi.lib 
   d3dcompiler.lib
   Shlwapi.lib
   dxguid.lib
)




