set(INSAR_MEX_TARGETS
    qualityMapGuideMethod_mex
    circularMedianFilter_mex
    branchCut_mex
    mcf_mex
)

matlab_add_mex(
    NAME    qualityMapGuideMethod_mex
    SRC     ${CMAKE_CURRENT_SOURCE_DIR}/qualityMapGuideMethod_mex.cpp
    LINK_TO unwrapped_phase
)

target_include_directories(qualityMapGuideMethod_mex PRIVATE ${PROJECT_SOURCE_DIR}/src)

matlab_add_mex(
    NAME    circularMedianFilter_mex
    SRC     ${CMAKE_CURRENT_SOURCE_DIR}/circularMedianFilter_mex.cpp
    LINK_TO unwrapped_phase
)

target_include_directories(circularMedianFilter_mex PRIVATE ${PROJECT_SOURCE_DIR}/src)

matlab_add_mex(
    NAME    branchCut_mex
    SRC     ${CMAKE_CURRENT_SOURCE_DIR}/branchCut_mex.cpp
    LINK_TO unwrapped_phase
)

target_include_directories(branchCut_mex PRIVATE ${PROJECT_SOURCE_DIR}/src)

matlab_add_mex(
    NAME    mcf_mex
    SRC     ${CMAKE_CURRENT_SOURCE_DIR}/mcf_mex.cpp
    LINK_TO unwrapped_phase
)

target_include_directories(mcf_mex PRIVATE ${PROJECT_SOURCE_DIR}/src)

if(INSAR_MEX_ENABLE_SNAPHU)
    foreach(target IN LISTS INSAR_MEX_TARGETS)
        target_compile_definitions(${target} PRIVATE INSAR_MEX_ENABLE_SNAPHU=1)
    endforeach()
endif()

add_custom_target(all_mex DEPENDS ${INSAR_MEX_TARGETS})
