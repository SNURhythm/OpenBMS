cmake_minimum_required(VERSION 3.10)
project(MyProject VERSION 1.0 LANGUAGES C CXX)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Compiler options
add_compile_options(-Wno-strict-prototypes -Wno-unused-but-set-variable -pedantic -Wno-ignored-attributes -Wstrict-aliasing -Wstrict-overflow -g)

# Include directories
include_directories(include)
include_directories(bgfx/bgfx/include)
include_directories(bgfx/bx/include)
include_directories(bgfx/bimg/include)
include_directories(bgfx/bgfx/3rdparty)

if(MINGW)
    include_directories(bgfx/bx/include/compat/mingw)
endif()

if(UNIX AND NOT APPLE)
    include_directories(bgfx/bx/include/compat/linux)
endif()

if(APPLE AND NOT IOS)
    include_directories(bgfx/bx/include/compat/osx)
endif()

if(IOS)
    include_directories(bgfx/bx/include/compat/ios)
endif()

if(MSVC)
    include_directories(bgfx/bx/include/compat/msvc)
endif()

# Source directory
set(SRC_DIR src)

# Build directory
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)

# Libraries directory
set(LIB_DIR lib)

# Platform-specific settings
if(WIN32)
    set(CP_ASSETS "xcopy /s /i assets ${BUILD_DIR}\\assets /Y")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-DAMD64)
    else()
        add_definitions(-DIA32)
    endif()
    link_directories(${LIB_DIR}/win32)

    find_library(SDL2_LIB SDL2 PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    find_library(SDL2MAIN_LIB SDL2main PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    find_library(SDL2_TFF_LIB SDL2_ttf PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    set(MINGW32_LIB mingw32)
    
    find_library(VLC_LIB vlc PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    find_library(VLCCORE_LIB vlccore PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    # glob all the dlls
    file(GLOB DLLS ${LIB_DIR}/win32/*.dll)
    set(VLC_PLUGINS_TAR libvlcplugins/win32.tar.gz)
elseif(UNIX AND NOT APPLE)
    set(CP_ASSETS "cp -r assets ${BUILD_DIR}")
    set(SDL2_LIB SDL2)
    set(SDL2MAIN_LIB SDL2main)
    set(SDL2_TFF_LIB SDL2_ttf)
    set(VLC_LIB vlc)
    set(VLCCORE_LIB vlccore)
    file(GLOB DLLS ${LIB_DIR}/linux/*.so)
    set(VLC_PLUGINS_TAR "")
elseif(APPLE)
    set(CP_ASSETS "cp -r assets ${BUILD_DIR}")
    find_library(SDL2_LIB SDL2 PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(SDL2MAIN_LIB SDL2main PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(SDL2_TFF_LIB SDL2_ttf PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(VLC_LIB vlc PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(VLCCORE_LIB vlccore PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH "@executable_path/lib")
    file(GLOB DLLS ${LIB_DIR}/darwin/*.dylib)
    set(VLC_PLUGINS_TAR libvlcplugins/darwin.tar.gz)
    find_library(FOUNDATION Foundation)
    file(GLOB_RECURSE MM_SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.mm)
    if(NOT FOUNDATION)
        message(FATAL_ERROR "Foundation framework not found")
    endif()
    # Add the Foundation framework to the linking process
    set(EXTRA_LIBS ${EXTRA_LIBS} ${FOUNDATION})
endif()

add_library(TinyFileDialogs STATIC ${SRC_DIR}/tinyfiledialogs.c)
add_library(SQLite3 STATIC ${SRC_DIR}/sqlite3.c)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)

# Main executable
add_executable(main ${SRC_FILES} ${MM_SRC_FILES})
target_link_libraries(main ${MINGW32_LIB} ${SDL2MAIN_LIB} ${SDL2_LIB} ${SDL2_TFF_LIB} ${VLC_LIB} ${VLCCORE_LIB} TinyFileDialogs SQLite3 ${EXTRA_LIBS} bgfx)

# Custom target for copying assets
add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
)

# Copy *.dll / *.so / *.dylib files
if(NOT "${DLLS}" STREQUAL "")
    add_custom_command(
        TARGET main PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${DLLS} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# Copy VLC plugins
if(NOT "${VLC_PLUGINS_TAR}" STREQUAL "")
    add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_SOURCE_DIR}/${VLC_PLUGINS_TAR}
        WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# Copy shaders
add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
)

# Installation rules
install(TARGETS main DESTINATION bin)
install(DIRECTORY assets DESTINATION ${BUILD_DIR})

# Additional targets for different build types like Debug, Release, etc.

add_subdirectory(bgfx)
