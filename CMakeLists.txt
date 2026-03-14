cmake_minimum_required(VERSION 4.0)
project(Radar_Algorithm)

set(CMAKE_CXX_STANDARD 23)

set(Matlab_ROOT_DIR "C:/Program Files/MATLAB/R2024b")
set(MATLAB_FIND_DEBUG ON)
find_package(Matlab REQUIRED COMPONENTS MEX_COMPILER MAIN_PROGRAM)

set(SOURCES
        src/phaseUnwrap/qualityMapGuideMethod.cpp
        src/utils/utils.cpp
        src/phaseUnwrap/branchCut.cpp
        src/phaseUnwrap/branchCut.h
)

file(GLOB_RECURSE HEADERS "src/*.h")

add_library(unwrapped_phase  STATIC ${SOURCES} ${HEADERS})

add_executable(Radar_main main.cpp)
target_link_libraries(Radar_main unwrapped_phase)
target_include_directories(Radar_main PRIVATE src)

matlab_add_mex(
        NAME        qualityMapGuideMethod_mex
        SRC         mex/qualityMapGuideMethod_mex.cpp
        LINK_TO     unwrapped_phase
)

