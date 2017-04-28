#================================
# Directory variables.
#================================
set(PROJ_ROOT "${ANIM_SOURCE_DIR}/project")
set(PROJ_INCLUDE_ROOT "${PROJ_ROOT}/include")
set(PROJ_SOURCE_ROOT "${PROJ_ROOT}/source")
set(PROJ_SHADER_ROOT "${PROJ_ROOT}/shaders")
set(PROJ_GENERATED_ROOT "${PROJ_ROOT}/generated")

#================================
# Add subdirectories.
#================================
add_subdirectory(${PROJ_INCLUDE_ROOT})
add_subdirectory(${PROJ_SOURCE_ROOT})
add_subdirectory(${PROJ_SHADER_ROOT})

#================================
# Include directories.
#================================
include_directories(
    ${ATLAS_INCLUDE_DIRS}
    ${PROJ_INCLUDE_ROOT}
    ${PROJ_GENERATED_ROOT})

#================================
# Source groups.
#================================
source_group("include" FILES ${INCLUDE_LIST})
source_group("source" FILES ${SOURCE_LIST})
source_group("shaders" FILES ${SHADER_LIST})

add_executable(raindrops ${SOURCE_LIST} ${INCLUDE_LIST} ${SHADER_LIST})
target_link_libraries(raindrops ${ATLAS_LIBRARIES})

