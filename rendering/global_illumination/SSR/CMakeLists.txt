add_executable(ssr ssr.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(ssr ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} 
common)