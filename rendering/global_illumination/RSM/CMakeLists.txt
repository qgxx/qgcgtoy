add_executable(rsm rsm.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(rsm ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} 
common)