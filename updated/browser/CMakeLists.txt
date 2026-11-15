cmake_minimum_required(VERSION 3.22)
project(cutout_browser LANGUAGES CXX)
set(CUTOUT_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(CUTOUT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_compile_options(-fexceptions)
add_subdirectory(../native native)
if(NOT EMSCRIPTEN)
    add_executable(cutout_bridge_reference bridge.cpp tests/reference.cpp)
    set_target_properties(cutout_bridge_reference PROPERTIES CXX_STANDARD 17)
    target_link_libraries(cutout_bridge_reference PRIVATE cutout_core)
    return()
endif()
add_executable(cutout_wasm bridge.cpp)
set_target_properties(cutout_wasm PROPERTIES CXX_STANDARD 17 SUFFIX ".js")
target_link_libraries(cutout_wasm PRIVATE cutout_core)
target_link_options(cutout_wasm PRIVATE
    -fexceptions --no-entry
    "-sMODULARIZE=1" "-sEXPORT_NAME=createCutout" "-sENVIRONMENT=worker,node"
    "-sALLOW_MEMORY_GROWTH=1" "-sINITIAL_MEMORY=67108864" "-sMAXIMUM_MEMORY=1073741824"
    "-sSTACK_SIZE=4194304" "-sFILESYSTEM=0"
    "-sEXPORTED_FUNCTIONS=['_malloc','_free','_cutout_segment','_cutout_error']"
    "-sEXPORTED_RUNTIME_METHODS=['HEAPU8','UTF8ToString']")
