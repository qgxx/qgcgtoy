add_executable(model_load model_load.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(model_load ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} )

add_test(NAME TEST_model_load COMMAND model_load)


add_executable(deferred_shading deferred_shading.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c)

target_link_libraries(deferred_shading ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} )

add_test(NAME TEST_deferred_shading COMMAND deferred_shading)