cmake_minimum_required(VERSION 3.10)
project(MyProject VERSION 1.0 LANGUAGES CXX)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options
add_compile_options(-Wall -pedantic -Werror -Wno-ignored-attributes -Wstrict-aliasing -Wstrict-overflow)

# Include directories
include_directories(include)

# Source directory
set(SRC_DIR src)

# Build directory
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)

# Libraries directory
set(LIB_DIR lib)

# Platform-specific settings
if(WIN32)
    set(CP_RES "xcopy /s /i res ${BUILD_DIR}\\res /Y")
    add_definitions(-DWIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-DAMD64)
    else()
        add_definitions(-DIA32)
    endif()
    link_directories(${LIB_DIR}/win32)

    set(SDL2_LIB SDL2)
    set(SDL2MAIN_LIB SDL2main)
    set(MINGW32_LIB mingw32)
    
    find_library(VLC_LIB vlc PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    find_library(VLCCORE_LIB vlccore PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    # glob all the dlls
    file(GLOB DLLS ${LIB_DIR}/win32/*.dll)
    set(VLC_PLUGINS_TAR libvlcplugins/win32.tar.gz)
elseif(UNIX AND NOT APPLE)
    set(CP_RES "cp -r res ${BUILD_DIR}")
    add_definitions(-DLINUX)
    find_library(SDL2_LIB SDL2 PATHS ${LIB_DIR}/linux NO_DEFAULT_PATH)
    find_library(SDL2MAIN_LIB SDL2main PATHS ${LIB_DIR}/linux NO_DEFAULT_PATH)
    find_library(VLC_LIB vlc PATHS ${LIB_DIR}/linux NO_DEFAULT_PATH)
    find_library(VLCCORE_LIB vlccore PATHS ${LIB_DIR}/linux NO_DEFAULT_PATH)
    file(GLOB DLLS ${LIB_DIR}/linux/*.so)
    set(VLC_PLUGINS_TAR libvlcplugins/linux.tar.gz)
elseif(APPLE)
    set(CP_RES "cp -r res ${BUILD_DIR}")
    add_definitions(-DOSX)
    find_library(SDL2_LIB SDL2 PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(SDL2MAIN_LIB SDL2main PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(VLC_LIB vlc PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(VLCCORE_LIB vlccore PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH "@executable_path/lib")
    file(GLOB DLLS ${LIB_DIR}/darwin/*.dylib)
    set(VLC_PLUGINS_TAR libvlcplugins/darwin.tar.gz)
endif()

# Main executable
add_executable(main ${SRC_DIR}/main.cpp)
target_link_libraries(main ${MINGW32_LIB} ${SDL2MAIN_LIB} ${SDL2_LIB} ${VLC_LIB} ${VLCCORE_LIB})

# Custom target for copying resources
add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res
)

# Copy *.dll / *.so / *.dylib files
add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${DLLS} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Copy VLC plugins
add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/${VLC_PLUGINS_TAR}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Installation rules
install(TARGETS main DESTINATION bin)
install(DIRECTORY res DESTINATION ${BUILD_DIR})

# Additional targets for different build types like Debug, Release, etc.