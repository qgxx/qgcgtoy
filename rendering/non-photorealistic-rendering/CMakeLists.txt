add_executable(style_shading style_shading.cpp 
${PROJECT_SOURCE_DIR}/external/src/glad/src/glad.c 
${PROJECT_SOURCE_DIR}/external/src/imgui/imgui.cpp 
${PROJECT_SOURCE_DIR}/external/src/imgui/imgui_demo.cpp 
${PROJECT_SOURCE_DIR}/external/src/imgui/imgui_draw.cpp 
${PROJECT_SOURCE_DIR}/external/src/imgui/imgui_tables.cpp 
${PROJECT_SOURCE_DIR}/external/src/imgui/imgui_widgets.cpp 
${PROJECT_SOURCE_DIR}/external/src/imgui/backends/imgui_impl_glfw.cpp 
${PROJECT_SOURCE_DIR}/external/src/imgui/backends/imgui_impl_opengl3.cpp)

target_link_libraries(style_shading ${assimp-vc143-mt_LIBRARY} 
${glfw3_LIBRARY} 
${glm_LIBRARY} 
common)