cmake_minimum_required(VERSION 3.1...3.27)

project(
  Glimmer
  VERSION 1.0
  LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set( SHADER_FILES
    shaders/default_pixel.hlsl
    shaders/base_vertex.hlsl
    shaders/equirectangular_pixel.hlsl
    shaders/equirectangular_vertex.hlsl
)

set(VERTEX_SHADERS
    shaders/base_vertex.hlsl
    shaders/equirectangular_vertex.hlsl
)

set(PIXEL_SHADERS
    shaders/default_pixel.hlsl
    shaders/equirectangular_pixel.hlsl
)

set_source_files_properties( 
    ${VERTEX_SHADERS}

PROPERTIES 
    VS_SHADER_TYPE Vertex
    VS_SHADER_ENTRYPOINT VSMain
    VS_SHADER_MODEL 5.1
)

set_source_files_properties( 
    ${PIXEL_SHADERS}

PROPERTIES 
    VS_SHADER_TYPE Pixel
    VS_SHADER_ENTRYPOINT PSMain
    VS_SHADER_MODEL 5.1
)


FILE(GLOB SRCS_GLIMMER_CORE
   src/core/commandqueue.h
   src/core/commandqueue.cpp
   src/core/commandmanager.h
   src/core/commandmanager.cpp
   src/core/texturemanager.h
   src/core/texturemanager.cpp
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
   src/core/staticdecriptorheap.h
   src/core/staticdecriptorheap.cpp
   src/core/descriptorheapallocator.h
   src/core/descriptorheapallocator.cpp
   src/core/dynamicdescriptorheap.h
   src/core/dynamicdescriptorheap.cpp
   src/core/descriptortypes.h
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
   src/core/context.h
   src/core/context.cpp
)

FILE(GLOB SRCS_RESOURCES
   src/core/resources/stb_image.h
   src/core/resources/dds.h
   src/core/resources/DDSTextureLoader.h
   src/core/resources/DDSTextureLoader.cpp
   src/core/resources/gpuresource.h
   src/core/resources/pixelbuffer.h
   src/core/resources/pixelbuffer.cpp
   src/core/resources/byteaddressbuffer.h
   src/core/resources/byteaddressbuffer.cpp
   src/core/resources/texture.h
   src/core/resources/texture.cpp
   src/core/resources/depthbuffer.h
   src/core/resources/depthbuffer.cpp
   src/core/resources/colorbuffer.h
   src/core/resources/colorbuffer.cpp
   src/core/resources/uploadbuffer.h
   src/core/resources/uploadbuffer.cpp
   src/core/resources/gpubuffer.h
   src/core/resources/gpubuffer.cpp
   src/core/resources/readbackbuffer.h
   src/core/resources/readbackbuffer.cpp
   src/core/resources/samplerdesc.h
   src/core/resources/samplerdesc.cpp
)

FILE(GLOB SRCS_RESOURCES_ALLOCATOR
   src/core/resources/memoryallocator/linearallocator.h
   src/core/resources/memoryallocator/linearallocator.cpp
)

FILE(GLOB SRCS_GEOMETRY
   src/core/geometry/objloader.h
   src/core/geometry/objloader.cpp
   src/core/geometry/defaultgeometry.h
   src/core/geometry/defaultgeometry.cpp
   src/core/geometry/vertexformat.h
   src/core/geometry/vertexformat.cpp
   src/core/geometry/mesh.h
   src/core/geometry/mesh.cpp
   src/core/geometry/material.h
   src/core/geometry/material.cpp
   src/core/geometry/model.h
   src/core/geometry/model.cpp
)

FILE(GLOB SRCS_MATH
   src/core/mathematics/bitoperation.h
)

FILE(GLOB SRCS_TYPES
   src/core/types/color.h
   src/core/types/commontypes.h
)

FILE(GLOB SRCS_COMPONENTS
   src/core/components/skybox.h
   src/core/components/skybox.cpp
   src/core/components/hdrtocubemap.h
   src/core/components/hdrtocubemap.cpp
   src/core/components/scene.h
   src/core/components/scene.cpp
   src/core/components/camera.h
   src/core/components/camera.cpp
   src/core/components/controller.h
   src/core/components/controller.cpp
)

set(SRCS_MAIN  
   src/main.cpp
)

add_executable(Glimmer WIN32 ${SRCS_MAIN} 
${SRCS_GLIMMER_CORE} 
${SRCS_RESOURCES_ALLOCATOR} 
${SRCS_TYPES}  
${SRCS_MATH}  
${SRCS_RESOURCES} 
${SRCS_GEOMETRY} 
${SRCS_COMPONENTS} 
${SHADER_FILES})

source_group("source" FILES ${SRCS_MAIN})
source_group("source\\core" FILES ${SRCS_GLIMMER_CORE})
source_group("source\\core\\resources" FILES ${SRCS_RESOURCES})
source_group("source\\core\\resources\\memoryallocator" FILES ${SRCS_RESOURCES_ALLOCATOR})
source_group( "source\\core\\types" FILES ${SRCS_TYPES} )
source_group( "source\\core\\mathematics" FILES ${SRCS_MATH} )
source_group( "source\\core\\geometry" FILES ${SRCS_GEOMETRY} )
source_group( "source\\core\\components" FILES ${SRCS_COMPONENTS} )
source_group( "resources\\shaders" FILES ${SHADER_FILES} )

set(LIB_DIR "${PROJECT_SOURCE_DIR}/dependences/lib")
set(BIN_DIR "${PROJECT_SOURCE_DIR}/dependences/bin")

target_include_directories(Glimmer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src/core
        ${PROJECT_SOURCE_DIR}/dependences/include
)

target_link_libraries(Glimmer 
   d3d12.lib 
   dxgi.lib 
   d3dcompiler.lib
   Shlwapi.lib
   dxguid.lib
   ${LIB_DIR}/assimp/assimp-vc143-mt.lib
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${BIN_DIR}/assimp/assimp-vc143-mt.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>)



