cmake_minimum_required(VERSION 3.21)

include(ExternalProject)
project(OpenBMS VERSION 1.0 LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#find_package(bgfx CONFIG REQUIRED)
set(BUILD_PROGRAMS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)


# Clear configuration-specific output settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Include directories
include_directories(include)
include_directories(bgfx/bgfx/include)
include_directories(bgfx/bx/include)
include_directories(bgfx/bimg/include)
include_directories(bgfx/bgfx/3rdparty)
#include_directories(libsndfile/include)
if (MINGW)
    include_directories(bgfx/bx/include/compat/mingw)
endif ()

if (UNIX AND NOT APPLE)
    include_directories(bgfx/bx/include/compat/linux)
endif ()

if (APPLE AND NOT IOS)
    include_directories(bgfx/bx/include/compat/osx)
endif ()

if (IOS)
    include_directories(bgfx/bx/include/compat/ios)
endif ()

if (MSVC)
    include_directories(bgfx/bx/include/compat/msvc)
endif ()

# Add Yoga include directories
include_directories(yoga)
include_directories(yoga/lib)

# Source directory
set(SRC_DIR src)

# Build directory
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)

# Libraries directory
set(LIB_DIR lib)
add_definitions(-DBX_CONFIG_DEBUG=0 -DSOL_LUAJIT=1 -DNOMINMAX)
set(FFMPEG_LIBS
        avcodec
        avfilter
        avformat
        avutil
        avdevice
        swresample
        swscale
        postproc
        x264
)
# Platform-specific settings
# for msvc
if (MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor /wd4819 /utf-8)
else ()
    add_compile_options(-Wno-strict-prototypes -Wno-unused-but-set-variable -pedantic -Wno-ignored-attributes -Wstrict-aliasing -Wstrict-overflow -g)
endif ()
if (WIN32)

    add_definitions(-DSTBI_WINDOWS_UTF8)
    find_package(SndFile CONFIG REQUIRED)
    set(CP_ASSETS "xcopy /s /i assets ${BUILD_DIR}\\assets /Y")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_definitions(-DAMD64)
    else ()
        add_definitions(-DIA32)
    endif ()
    link_directories(${LIB_DIR}/win32)

    find_library(SDL2_LIB SDL2 PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH REQUIRED)
    # add dll
    add_library(SDL2 SHARED IMPORTED GLOBAL)
    # set IMPORTED_LOCATION and IMPORTED_IMPLIB:
    set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/win32/SDL2.dll IMPORTED_IMPLIB ${LIB_DIR}/win32/SDL2.lib)
    find_library(SDL2MAIN_LIB SDL2main PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    add_library(SDL2main SHARED IMPORTED)
    set_target_properties(SDL2main PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/win32/SDL2main.dll IMPORTED_IMPLIB ${LIB_DIR}/win32/SDL2main.lib)
    find_library(SDL2_TFF_LIB SDL2_ttf PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH)
    find_library(LUA_LIB lua51 PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH REQUIRED)
    find_library(LUAJIT_LIB luajit PATHS ${LIB_DIR}/win32 NO_DEFAULT_PATH REQUIRED)
    #set(MINGW32_LIB mingw32)

    # glob all the dlls
    file(GLOB DLLS ${LIB_DIR}/win32/*.dll)
    set(EXTRA_LIBS SndFile::sndfile)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")

elseif (UNIX AND NOT APPLE)
    set(CP_ASSETS "cp -r assets ${BUILD_DIR}")
    set(SDL2_LIB SDL2)
    set(SDL2MAIN_LIB SDL2main)
    set(SDL2_TFF_LIB SDL2_ttf)
    find_library(FFMPEG_LIBS avcodec avfilter avformat avutil avdevice swresample swscale postproc vpx x264 x265 PATHS ${LIB_DIR}/linux NO_DEFAULT_PATH)
    file(GLOB DLLS ${LIB_DIR}/linux/*.so)
    set(EXTRA_LIBS X11 sndfile)
    add_definitions(-DSDL_VIDEO_DRIVER_X11=ON)
elseif (APPLE)
    link_directories(${LIB_DIR}/darwin)
    find_package(SndFile CONFIG REQUIRED)
    set(CP_ASSETS "cp -r assets ${BUILD_DIR}")
    find_library(SDL2_LIB SDL2 PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(SDL2MAIN_LIB SDL2main PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(SDL2_TFF_LIB SDL2_ttf PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(LUAJIT_LIB luajit PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    find_library(FFMPEG_LIBS NAMES avcodec avfilter avformat avutil avdevice swresample swscale postproc vpx x264 x265 PATHS ${LIB_DIR}/darwin NO_DEFAULT_PATH)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_RPATH "@executable_path/lib")
    file(GLOB DLLS ${LIB_DIR}/darwin/*.dylib)
    find_library(FOUNDATION Foundation)
    find_library(AUDIO_TOOLBOX AudioToolbox)
    find_library(VIDEO_TOOLBOX VideoToolbox)
    find_library(CORE_AUDIO CoreAudio)
    find_library(AV_KIT AVKit)
    find_library(CORE_IMAGE CoreImage)
    find_library(CORE_MEDIA CoreMedia)
    find_library(ACCELERATE Accelerate)
    find_library(OPENGL OpenGL)
    find_library(OPENCL OpenCL)
    find_library(COCOA Cocoa)
    find_library(METAL Metal)
    find_library(SECURITY Security)
    if (NOT FOUNDATION)
        message(FATAL_ERROR "Foundation framework not found")
    else ()
        message(STATUS "Foundation framework found: ${FOUNDATION}")
    endif ()
    if (NOT FFMPEG_LIBS)
        message(FATAL_ERROR "FFmpeg libraries not found")
    else ()
        message(STATUS "FFmpeg libraries found: ${FFMPEG_LIBS}")
    endif ()
    # Add the Foundation framework to the linking process
    set(EXTRA_LIBS ${EXTRA_LIBS} ${FOUNDATION} ${AUDIO_TOOLBOX} ${VIDEO_TOOLBOX} ${CORE_AUDIO} ${AV_KIT} ${CORE_IMAGE} ${CORE_MEDIA} ${ACCELERATE} ${OPENGL} ${OPENCL} ${COCOA} ${METAL} ${SECURITY} SndFile::sndfile bz2 z iconv)
elseif (IOS)
    # TODO
    find_library(UIKIT UIKit)
    find_library(FOUNDATION Foundation)
    target_link_libraries(main PRIVATE ${UIKIT} ${FOUNDATION})
endif ()

if (WIN32)
    string(REPLACE ".lib" ".dll" SDL2_LIB_DLL ${SDL2_LIB})
    set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION ${SDL2_LIB_DLL} IMPORTED_IMPLIB ${SDL2_LIB})
else (WIN32)
    set_target_properties(SDL2 PROPERTIES IMPORTED_LOCATION ${SDL2_LIB})
endif (WIN32)

add_library(TinyFileDialogs STATIC ${SRC_DIR}/tinyfiledialogs.c)
add_library(SQLite3 STATIC ${SRC_DIR}/sqlite3.c)

# Main executable
add_executable(main)

target_link_libraries(main PRIVATE ${MINGW32_LIB} ${SDL2MAIN_LIB} ${SDL2_LIB} ${SDL2_TFF_LIB} ${FFMPEG_LIBS} ${LUA_LIB} ${LUAJIT_LIB} TinyFileDialogs SQLite3 ${EXTRA_LIBS} bgfx yogacore)

# Custom target for copying assets
add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
)

# Copy *.dll / *.so / *.dylib files
if (NOT "${DLLS}" STREQUAL "")
    add_custom_command(
            TARGET main PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${DLLS} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
    )
endif ()

# print $<TARGET_RUNTIME_DLLS:main> to console
#file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dlls.txt" CONTENT "$<TARGET_RUNTIME_DLLS:main>")

#install(FILES $<TARGET_RUNTIME_DLLS:main> TYPE BIN)


file(GLOB_RECURSE SHADER_FILES
        "${CMAKE_SOURCE_DIR}/shaders/*.*"
)

# Add a custom target that depends on all shader files
add_custom_target(copy_shaders ALL
        DEPENDS ${SHADER_FILES}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
)

# Make the main target depend on shader copying
add_dependencies(main copy_shaders)
# Installation rules
install(TARGETS main DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)

# Additional targets for different build types like Debug, Release, etc.

add_subdirectory(bgfx)
add_subdirectory(src)
add_subdirectory(yoga)
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
