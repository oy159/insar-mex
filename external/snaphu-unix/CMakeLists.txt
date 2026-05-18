if(NOT INSAR_MEX_BUILD_SNAPHU)
    return()
endif()

if(WIN32)
    message(WARNING "Bundled SNAPHU build is skipped on Windows because upstream sources rely on POSIX APIs (unistd, fork, wait, getrusage, etc.).")
    add_library(snaphu INTERFACE)
    target_compile_definitions(snaphu INTERFACE INSAR_MEX_SNAPHU_UNAVAILABLE=1)
    return()
endif()

set(SNAPHU_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu_cost.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu_cs2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu_io.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu_solver.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu_tile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snaphu_util.c
)

add_executable(snaphu ${SNAPHU_SOURCES})

target_include_directories(snaphu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(UNIX)
    target_link_libraries(snaphu PRIVATE m)
endif()

set_target_properties(snaphu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
