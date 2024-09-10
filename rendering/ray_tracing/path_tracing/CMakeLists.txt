add_executable(path_tracing path_tracing.cpp)

target_link_libraries(path_tracing ${glm_LIBRARY})


add_executable(bvh bvh.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(bvh ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY})