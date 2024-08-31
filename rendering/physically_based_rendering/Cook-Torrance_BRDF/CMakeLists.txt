add_executable(cook-torrance_brdf cook-torrance_brdf.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(cook-torrance_brdf ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} 
common)