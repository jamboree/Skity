# add_shader depends on Vulkan::glslc. if not found, guide the user to find vulkan and the executable.
function(add_shader RET OUTPUT_DIR)
    if(NOT Vulkan_FOUND)
        message(FATAL_ERROR
            "The addShader() function depends on the \"glslc\" shader compiler executable.\n"
            "It is detected during find_package(Vulkan REQUIRED) and defined as imported target executable Vulkan::glslc.\n"
            "Please use find_package(Vulkan REQUIRED) and make sure it succeeds!\n"
        )
    endif()
    foreach(SOURCE_FILE IN LISTS ARGN)
        get_filename_component(INPUT_EXT "${SOURCE_FILE}" LAST_EXT)
        message("add_shader: ${SOURCE_FILE}")
	    get_filename_component(INPUT_PATH "${SOURCE_FILE}" ABSOLUTE)
        get_filename_component(INPUT_FILENAME "${SOURCE_FILE}" NAME)

        set(OUTPUT_FILENAME "${INPUT_FILENAME}.spv")
        set(OUTPUT_PATH "${OUTPUT_DIR}/${OUTPUT_FILENAME}")

        # Create the output directory.
        file(MAKE_DIRECTORY "${OUTPUT_DIR}")

	    # Add a custom command to compile GLSL to SPIR-V.
	    add_custom_command(
		    OUTPUT "${OUTPUT_PATH}"
		    COMMAND Vulkan::glslc -I "${CMAKE_CURRENT_SOURCE_DIR}" -o "${OUTPUT_PATH}" "${INPUT_PATH}"
		    DEPENDS "${INPUT_PATH}"
		    VERBATIM)

        set(OUTPUT_PATHS "${OUTPUT_PATHS}" "${OUTPUT_PATH}")
    endforeach()

    set(${RET} "${OUTPUT_PATHS}" PARENT_SCOPE)
endfunction()


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
    file(APPEND ${output} "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
  endforeach()
endfunction()

add_shader(shader_objects ${CMAKE_CURRENT_BINARY_DIR}/shadersXX
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vk/vk_uniform_color.frag
)
create_resources(${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shader.hpp)
