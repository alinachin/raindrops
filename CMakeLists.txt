# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.0)

project(uvic-animation)

# Tell IDE to use folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#================================
# Directory variables.
#================================
set(ANIM_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
set(ANIM_BINARY_DIR "${PROJECT_BINARY_DIR}")
set(ANIM_ATLAS_ROOT "${ANIM_SOURCE_DIR}/lib/atlas")
set(ANIM_ATLAS_CONFIG_ROOT "${ANIM_ATLAS_ROOT}/config")

#================================
# Compilation flags.
#================================
# Borrow the setup from Atlas.
include("${ANIM_ATLAS_CONFIG_ROOT}/Compiler.cmake")

#================================
# Add subdirectories.
#================================
# Add Atlas
add_subdirectory("${ANIM_ATLAS_ROOT}")

# Now add the lab subdirectories.
add_subdirectory("${ANIM_SOURCE_DIR}/project")
