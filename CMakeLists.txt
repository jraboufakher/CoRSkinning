cmake_minimum_required(VERSION 3.12)
project(CoRSkinning)

# Use C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable pthread for std::async
if (NOT WIN32)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")
endif()

# Base path for third-party on Windows
set(LIBRARIES_BASE_PATH "C:/ThirdPartyLibraries")

# Public includes (your own headers + GLM + FBX SDK)
set(PUBLIC_INCLUDES
    ${CMAKE_SOURCE_DIR}/include
    "${LIBRARIES_BASE_PATH}/GL/GLM/glm-0.9.9.7"
    "${LIBRARIES_BASE_PATH}/FBX/include"
    "${LIBRARIES_BASE_PATH}/GL/FreeGLUT/include"
    "${LIBRARIES_BASE_PATH}/GL/GLEW/include"
    "${LIBRARIES_BASE_PATH}/GL/GLFW/include"
    "${LIBRARIES_BASE_PATH}/opencv/build/include"
)
include_directories(${PUBLIC_INCLUDES})

# Link directories for FreeGLUT & GLEW & GLFW
link_directories(
    "${LIBRARIES_BASE_PATH}/GL/FreeGLUT/lib/x64"
    "${LIBRARIES_BASE_PATH}/GL/GLEW/lib/x64"
    "${LIBRARIES_BASE_PATH}/GL/GLFW/lib"
    "${LIBRARIES_BASE_PATH}/opencv/build/x64/vc16/lib"
)

# FBXLib target
set(FBX_HEADERS 
    include/FBXLoader.h
)
set(FBX_SOURCES 
    src/FBXLoader.cpp
)
add_library(FBXLib ${FBX_HEADERS} ${FBX_SOURCES})

# Link FBX SDK libs
target_link_libraries(FBXLib PUBLIC
    "${LIBRARIES_BASE_PATH}/FBX/lib/libfbxsdk-md.lib"
    "${LIBRARIES_BASE_PATH}/FBX/lib/libxml2-md.lib"
    "${LIBRARIES_BASE_PATH}/FBX/lib/zlib-md.lib"
    "${LIBRARIES_BASE_PATH}/FBX/lib/unzip-md.lib"
)

# CoRLib target
set(CoR_HEADERS
    include/cor/Clock.h
    include/cor/CoRCalculator.h
    include/cor/CoRMesh.h
    include/cor/CoRTriangle.h
    include/cor/WeightsPerBone.h
    include/cor/Mesh.h
    include/cor/ShaderUtils.h
)
set(CoR_SOURCES
    src/cor/Clock.cpp
    src/cor/CoRCalculator.cpp
    src/cor/CoRTriangle.cpp
    src/cor/WeightsPerBone.cpp
    src/cor/Mesh.cpp
    src/cor/ShaderUtils.cpp
)
add_library(CoRLib ${CoR_HEADERS} ${CoR_SOURCES})

# Executable target
add_executable(CoRSkinning main.cpp)

# Link order: your libs first, then OpenGL, then GLEW/FreeGLUT
find_package(OpenGL REQUIRED)
target_link_libraries(CoRSkinning PRIVATE
    FBXLib
    CoRLib
    ${OPENGL_gl_LIBRARY}
    glew32         # GLEW
    freeglut       # FreeGLUT import lib
    glfw3          # GLFW
    opencv_world4110d # OpenCV
)

# Post-build: copy FBX SDK DLL
add_custom_command(TARGET CoRSkinning POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRARIES_BASE_PATH}/FBX/dll/libfbxsdk.dll"
        $<TARGET_FILE_DIR:CoRSkinning>
)

# Copy GLEW DLL
add_custom_command(TARGET CoRSkinning POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRARIES_BASE_PATH}/GL/GLEW/bin/glew32.dll"
        $<TARGET_FILE_DIR:CoRSkinning>
)

# Copy FreeGLUT DLL
add_custom_command(TARGET CoRSkinning POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRARIES_BASE_PATH}/GL/FreeGLUT/bin/freeglut.dll"
        $<TARGET_FILE_DIR:CoRSkinning>
)

# Copy GLFW DLL
add_custom_command(TARGET CoRSkinning POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRARIES_BASE_PATH}/GL/GLFW/bin/glfw3.dll"
        $<TARGET_FILE_DIR:CoRSkinning>
)

# Copy OpenCV DLL
add_custom_command(TARGET CoRSkinning POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIBRARIES_BASE_PATH}/opencv/build/x64/vc16/bin/opencv_world4110d.dll"
        $<TARGET_FILE_DIR:CoRSkinning>
)