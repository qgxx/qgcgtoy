# Results

## Shadow
You can drag the left mouse button to rotate the camera,   
and hold down Ctrl or Shift key while dragging the left mouse button to move the scenes.  

### ShadowMapping
![ShadowMapping](./assets/results/SM.png)

### Percentage Closer Filering
![PCF](./assets/results/PCF.png)

### Percentage Closer Soft Shadow
![PCSS](./assets/results/PCSS.png)  

## Environment Mapping

### Precomputed Radiance Transfer
![PRT](./assets/results/PRT.png)

### Imaged Based Lighting(diffuse)
![IBL-diffuse](./assets/results/IBL-diffuse.gif)

### Imaged Based Lighting(specular)
![IBL-specular](./assets/results/IBL-specular.gif)

## Global Illumination

### Screen Space Ambient Occlusion 
You can move camera by `w`,`s`,`a`,`d`,
and drag the left mouse button to rotate the camera.
![SSAO](./assets/results/SSAO.png)

## Physically Based Rendering

### Cook-Torrance BRDF PBR
![Cook-Torrance BRDF](./assets/results/Cook-Torrance%20BRDF.gif)

# External Dependencies
Assimp  
glfw  
glad  
glm  
imgui  
nori  
stb_image

# Run
After making sure all dependencies are downloaded, start `Build External Dependencies`task to process them.  
Start `Build Rendering`task to compile the all project.  
Please refer to the `task.json` file for detailed configuration.

---
# References
[GAMES202-Real-Time High Quality Rendering](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)  
[花桑's GAMES202 Assignment](https://www.zhihu.com/column/c_1591546501603545090)  
[WC Yang's Real-Time High Quality Rendering](https://yangwc.com/)   
[KillerAery's Real-time Global Illumination](https://www.cnblogs.com/KillerAery/collections/3076)  
[LearnOpenGL](https://learnopengl.com/)  
