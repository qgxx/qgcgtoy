add_executable(forward_plus forward_plus.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(forward_plus ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} )