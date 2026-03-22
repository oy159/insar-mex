cmake_minimum_required(VERSION 4.0)
project(Radar_Algorithm)

set(CMAKE_CXX_STANDARD 23)

set(Matlab_ROOT_DIR "C:/Program Files/MATLAB/R2024b")
set(MATLAB_FIND_DEBUG ON)
find_package(Matlab REQUIRED COMPONENTS MEX_COMPILER MAIN_PROGRAM)


add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(SOURCES
        src/phaseUnwrap/qualityMapGuideMethod.cpp
        src/utils/utils.cpp
        src/phaseUnwrap/branchCut.cpp
        src/filter/CircularMedianFilter.cpp
)

file(GLOB_RECURSE HEADERS "src/*.h")

add_library(unwrapped_phase STATIC ${SOURCES} ${HEADERS})

add_executable(Radar_main main.cpp)
target_link_libraries(Radar_main unwrapped_phase)
target_include_directories(Radar_main PRIVATE src)

matlab_add_mex(
        NAME        qualityMapGuideMethod_mex
        SRC         mex/qualityMapGuideMethod_mex.cpp
        LINK_TO     unwrapped_phase
)

matlab_add_mex(
        NAME        circularMedianFilter_mex
        SRC         mex/circularMedianFilter_mex.cpp
        LINK_TO     unwrapped_phase
)

matlab_add_mex(
        NAME        branchCut_mex
        SRC         mex/branchCut_mex.cpp
        LINK_TO     unwrapped_phase
)


add_custom_target(all_mex DEPENDS qualityMapGuideMethod_mex circularMedianFilter_mex branchCut_mex)

