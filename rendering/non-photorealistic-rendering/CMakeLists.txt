add_executable(style_shading style_shading.cpp)

target_link_libraries(style_shading ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} 
common 
imgui)