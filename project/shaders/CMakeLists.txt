set(SHADER_SOURCE
    "${PROJ_SHADER_ROOT}/grid.fs.glsl"
	"${PROJ_SHADER_ROOT}/grid.vs.glsl"
	"${PROJ_SHADER_ROOT}/hmap.fs.glsl"
	"${PROJ_SHADER_ROOT}/hmap.vs.glsl"
	"${PROJ_SHADER_ROOT}/particle.fs.glsl"
	"${PROJ_SHADER_ROOT}/particle.vs.glsl"
	"${PROJ_SHADER_ROOT}/idmap.fs.glsl"
	"${PROJ_SHADER_ROOT}/idmap.vs.glsl")

set_source_files_properties(${SHADER_SOURCE} 
    PROPERTIES HEADER_FILE_ONLY TRUE)

set(SHADER_INCLUDE "${PROJ_GENERATED_ROOT}/ShaderPaths.hpp")

configure_file(
    "${PROJ_SHADER_ROOT}/ShaderPaths.hpp.in"
    ${SHADER_INCLUDE})

set(SHADER_LIST
    ${SHADER_SOURCE}
    ${SHADER_INCLUDE}
    PARENT_SCOPE)

