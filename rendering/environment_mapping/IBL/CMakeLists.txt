add_executable(ibl ibl.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(ibl ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} 
common)