cmake_minimum_required(VERSION 3.20)

# Default install to <project_root>/install (skip if user explicitly set a prefix)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
   OR CMAKE_INSTALL_PREFIX MATCHES "^C:/Program Files"
   OR CMAKE_INSTALL_PREFIX MATCHES "^/usr/local")
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "" FORCE)
endif()

project(insar_mex LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

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
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/insar_mex>
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

# ── Install rules ──────────────────────────────────────────────────────────
# Collect targets to install
set(INSAR_MEX_INSTALL_TARGETS unwrapped_phase Radar_main)
if(INSAR_MEX_BUILD_SNAPHU AND TARGET snaphu)
    list(APPEND INSAR_MEX_INSTALL_TARGETS snaphu)
endif()
if(INSAR_MEX_BUILD_SNAPHU AND TARGET snaphu_cli)
    list(APPEND INSAR_MEX_INSTALL_TARGETS snaphu_cli)
endif()

install(TARGETS ${INSAR_MEX_INSTALL_TARGETS}
    EXPORT  insar_mexTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# MEX targets (shared libraries, install to mex/)
if(Matlab_FOUND)
    set(INSAR_MEX_MEX_TARGETS
        qualityMapGuideMethod_mex
        circularMedianFilter_mex
        branchCut_mex
    )
    if(TARGET snaphu_mex)
        list(APPEND INSAR_MEX_MEX_TARGETS snaphu_mex)
    endif()
    install(TARGETS ${INSAR_MEX_MEX_TARGETS}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/mex
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mex
    )
endif()

# README
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/install/README.md
    DESTINATION .
)

# Public headers
install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/insar_mex
    FILES_MATCHING PATTERN "*.h"
)

# SNAPHU public header
if(INSAR_MEX_BUILD_SNAPHU AND TARGET snaphu)
    install(FILES external/snaphu-win/src/snaphu.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/insar_mex/phaseUnwrap
    )
endif()

# CMake package config for find_package(insar_mex)
install(EXPORT insar_mexTargets
    FILE        insar_mexTargets.cmake
    NAMESPACE   insar_mex::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/insar_mex
)

include(CMakePackageConfigHelpers)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/insar_mexConfig.cmake
[=[include(CMakeFindDependencyMacro)
include("${CMAKE_CURRENT_LIST_DIR}/insar_mexTargets.cmake")
check_required_components(insar_mex)
]=])

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/insar_mexConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/insar_mexConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/insar_mexConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/insar_mex
)
