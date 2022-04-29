set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vk)
file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss)

foreach(SHADER IN LISTS SHADERS)
    get_filename_component(FILENAME ${SHADER} NAME)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
        COMMAND Vulkan::glslc -I ${SHADER_DIR} ${SHADER} -o ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv
        DEPENDS ${SHADER}
        COMMENT "Compiling ${FILENAME}")
list(APPEND SPV_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME}.spv)
endForeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

# Creates C resources file from files in given directory
function(create_resources dir output)
  # Create empty output file
  file(WRITE ${output} "#pragma once\n")
  # Collect input files
  file(GLOB bins ${dir}/*.glsl ${dir}/*.spv)
  # Iterate through input files
  foreach(bin ${bins})
    message("bin: ${bin}")
    # Get short filename
    string(REGEX MATCH "([^/]+)$" filename ${bin})
    # Replace filename spaces & extension separator for C compatibility
    string(REGEX REPLACE "\\.| |-" "_" filename ${filename})
    # Read hex data from file
    file(READ ${bin} filedata HEX)
    # Convert hex data for C compatibility
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
    # Append data to output file
    file(APPEND ${output} "alignas(4) const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
  endforeach()
endfunction()

create_resources(${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shader.hpp)
