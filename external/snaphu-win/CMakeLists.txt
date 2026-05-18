# snaphu-win: Windows port of SNAPHU phase unwrapping
# Uses a single amalgamation file (snaphu_wrapper.c) that includes all sources

add_library(snaphu STATIC
    snaphu_wrapper.c
)

target_include_directories(snaphu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(snaphu PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(snaphu PRIVATE /W1 /wd4244 /wd4267 /wd4996)
    target_compile_definitions(snaphu PRIVATE _CRT_SECURE_NO_WARNINGS _USE_MATH_DEFINES)
endif()
