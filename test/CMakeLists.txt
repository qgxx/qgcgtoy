add_executable(PMX load_pmx.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(PMX ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} )

add_test(NAME TEST_PMX COMMAND PMX)