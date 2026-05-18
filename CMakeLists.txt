cmake_minimum_required(VERSION 3.20)
project(insar_mex LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(INSAR_MEX_BUILD_MEX "Build MATLAB MEX targets" ON)
option(INSAR_MEX_ENABLE_SNAPHU "Enable SNAPHU integration hooks in insar-mex code" ON)
option(INSAR_MEX_BUILD_SNAPHU "Build bundled SNAPHU target from external/snaphu" ON)

set(Matlab_ROOT_DIR "" CACHE PATH "Path to MATLAB installation (used only when INSAR_MEX_BUILD_MEX=ON)")

add_compile_options(
    "$<$<C_COMPILER_ID:MSVC>:/utf-8>"
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
)

set(INSAR_MEX_CORE_SOURCES
    src/phaseUnwrap/qualityMapGuideMethod.cpp
    src/utils/utils.cpp
    src/phaseUnwrap/branchCut.cpp
    src/filter/CircularMedianFilter.cpp
)

file(GLOB_RECURSE INSAR_MEX_CORE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_subdirectory(external)

add_library(unwrapped_phase STATIC
    ${INSAR_MEX_CORE_SOURCES}
    ${INSAR_MEX_CORE_HEADERS}
)

target_include_directories(unwrapped_phase
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_compile_features(unwrapped_phase PUBLIC cxx_std_23)

if(INSAR_MEX_ENABLE_SNAPHU)
    target_compile_definitions(unwrapped_phase PUBLIC INSAR_MEX_ENABLE_SNAPHU=1)
endif()

add_executable(Radar_main main.cpp)
target_link_libraries(Radar_main PRIVATE unwrapped_phase)
target_include_directories(Radar_main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(INSAR_MEX_ENABLE_SNAPHU)
    target_compile_definitions(Radar_main PRIVATE INSAR_MEX_ENABLE_SNAPHU=1)
endif()

if(INSAR_MEX_BUILD_SNAPHU AND TARGET snaphu)
    target_link_libraries(Radar_main PRIVATE snaphu)
    target_compile_definitions(Radar_main PRIVATE INSAR_MEX_SNAPHU_AVAILABLE=1)
endif()

if(INSAR_MEX_BUILD_MEX)
    find_package(Matlab QUIET COMPONENTS MEX_COMPILER MAIN_PROGRAM)
    if(Matlab_FOUND)
        add_subdirectory(mex)
    else()
        message(WARNING "MATLAB not found. Skipping MEX targets. Set Matlab_ROOT_DIR or disable INSAR_MEX_BUILD_MEX.")
    endif()
endif()
