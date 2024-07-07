cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
project(MyProject VERSION 1.0 LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#find_package(bgfx CONFIG REQUIRED)
set(BUILD_PROGRAMS OFF)
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
#include_directories(libsndfile/include)
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
add_definitions(-DBX_CONFIG_DEBUG=1)
# Platform-specific settings
if(WIN32)
    find_package(SndFile CONFIG REQUIRED)
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
    set(EXTRA_LIBS SndFile::sndfile)
elseif(UNIX AND NOT APPLE)
    set(CP_ASSETS "cp -r assets ${BUILD_DIR}")
    set(SDL2_LIB SDL2)
    set(SDL2MAIN_LIB SDL2main)
    set(SDL2_TFF_LIB SDL2_ttf)
    set(VLC_LIB vlc)
    set(VLCCORE_LIB vlccore)
    file(GLOB DLLS ${LIB_DIR}/linux/*.so)
    set(VLC_PLUGINS_TAR "")
    set(EXTRA_LIBS X11 sndfile)
    add_definitions(-DSDL_VIDEO_DRIVER_X11=ON)
elseif(APPLE)
    find_package(SndFile CONFIG REQUIRED)
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
    set(EXTRA_LIBS ${EXTRA_LIBS} ${FOUNDATION} SndFile::sndfile)
elseif(IOS)
    # TODO
    find_library(UIKIT UIKit)
    find_library(FOUNDATION Foundation)
    target_link_libraries(main PRIVATE ${UIKIT} ${FOUNDATION})
endif()

add_library(TinyFileDialogs STATIC ${SRC_DIR}/tinyfiledialogs.c)
add_library(SQLite3 STATIC ${SRC_DIR}/sqlite3.c)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp)
# Main executable
add_executable(main ${SRC_FILES} ${MM_SRC_FILES})

target_link_libraries(main PRIVATE ${MINGW32_LIB} ${SDL2MAIN_LIB} ${SDL2_LIB} ${SDL2_TFF_LIB} ${VLC_LIB} ${VLCCORE_LIB} TinyFileDialogs SQLite3 ${EXTRA_LIBS} bgfx)


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
#add_subdirectory(libsndfile)
# # Add pre-generated Xcode projects as subprojects
# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# # Add bgfx.xcodeproj
# set(BGFX_PROJECT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bgfx/bgfx.xcodeproj")

# # Ensure bgfx is built before adding the custom target
# add_custom_command(
#     OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bgfx/bgfx.xcodeproj
#     COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target bgfx
#     DEPENDS bgfx
#     COMMENT "Waiting for bgfx.xcodeproj to be generated"
# )

# add_custom_target(bgfx_proj ALL
#     COMMAND ${CMAKE_COMMAND} -E echo "Adding bgfx.xcodeproj"
#     SOURCES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/bgfx/bgfx.xcodeproj
# )
# set_target_properties(bgfx_proj PROPERTIES FOLDER "External Projects")

# # Add SDL_ttf.xcodeproj
# set(SDL_TTF_PROJECT "${CMAKE_SOURCE_DIR}/SDL_ttf/Xcode/SDL_ttf.xcodeproj")
# add_custom_target(SDL_ttf_proj ALL
#     COMMAND ${CMAKE_COMMAND} -E echo "Adding SDL_ttf.xcodeproj"
#     SOURCES ${SDL_TTF_PROJECT}
# )
# set_target_properties(SDL_ttf_proj PROPERTIES FOLDER "External Projects")

# # Add SDL.xcodeproj
# set(SDL_PROJECT "${CMAKE_SOURCE_DIR}/SDL/Xcode/SDL/SDL.xcodeproj")
# add_custom_target(SDL_proj ALL
#     COMMAND ${CMAKE_COMMAND} -E echo "Adding SDL.xcodeproj"
#     SOURCES ${SDL_PROJECT}
# )
# set_target_properties(SDL_proj PROPERTIES FOLDER "External Projects")

# # Ensure dependencies are built before the main target
# add_dependencies(main bgfx_proj SDL_ttf_proj SDL_proj)
