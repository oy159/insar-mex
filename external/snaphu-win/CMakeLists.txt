# snaphu-win: Windows port of SNAPHU phase unwrapping

set(SNAPHU_SOURCES
    src/snaphu.c
    src/snaphu_cost.c
    src/snaphu_cs2.c
    src/snaphu_io.c
    src/snaphu_solver.c
    src/snaphu_tile.c
    src/snaphu_util.c
)

# Static library (in-memory API via snaphu_wrapper.c)
add_library(snaphu STATIC
    snaphu_wrapper.c
)

target_include_directories(snaphu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/insar_mex/phaseUnwrap>
)

set_target_properties(snaphu PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(snaphu PRIVATE /W1 /wd4244 /wd4267 /wd4996)
    target_compile_definitions(snaphu PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
endif()

# Standalone CLI executable (file-based I/O, original main())
add_executable(snaphu_cli ${SNAPHU_SOURCES})

target_include_directories(snaphu_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(snaphu_cli PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    OUTPUT_NAME snaphu
)

if(MSVC)
    target_compile_options(snaphu_cli PRIVATE /W1 /wd4244 /wd4267 /wd4996)
    target_compile_definitions(snaphu_cli PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
endif()
