add_subdirectory(audio)
add_subdirectory(rendering)
add_subdirectory(scene)
add_subdirectory(video)
add_subdirectory(view)
add_subdirectory(game)
add_subdirectory(math)
add_subdirectory(utils)
add_subdirectory(input)
target_sources(main PRIVATE bgfx_helper.cpp bms_parser.cpp ChartDBHelper.cpp main.cpp Utils.cpp path.cpp)
if (APPLE)
    target_sources(main PRIVATE MacNatives.mm)
endif ()