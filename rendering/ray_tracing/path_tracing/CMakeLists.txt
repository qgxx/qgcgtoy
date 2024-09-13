add_executable(path_tracing path_tracing.cpp)

target_link_libraries(path_tracing ${glm_LIBRARY})


add_executable(bvh bvh.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(bvh ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY})


add_executable(path_tracing_gpu path_tracing_gpu.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(path_tracing_gpu ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY})


add_executable(disney_principle disney_principle.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(disney_principle ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY})


add_executable(path_tracing_mis path_tracing_mis.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c 
${PROJECT_SOURCE_DIR}/framework/utils/load/hdrloader.cpp)

target_link_libraries(path_tracing_mis ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY})