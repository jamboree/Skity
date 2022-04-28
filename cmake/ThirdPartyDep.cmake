# glm
find_package(glm CONFIG REQUIRED)
# pugixml
if(${BUILD_SVG_MODULE})
    find_package(pugixml CONFIG REQUIRED)
endif()

if(${ENABLE_LOG})
    find_package(spdlog CONFIG REQUIRED)
    target_link_libraries(skity PRIVATE spdlog::spdlog)
endif()


# include glm
target_link_libraries(skity PUBLIC glm::glm)


# OpenGL header file
if(${OPENGL_BACKEND})
  target_include_directories(skity PRIVATE third_party/OpenGL)
endif()

# Vulkan memory allocator
if(${VULKAN_BACKEND})
    find_package(Vulkan REQUIRED)
    find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
    target_include_directories(skity PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(skity PUBLIC ${Vulkan_LIBRARIES})
    target_link_libraries(skity PRIVATE unofficial::vulkan-memory-allocator::vulkan-memory-allocator)
endif()
