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

### Screen Space Reflections
![SSR](./assets/results/SSR1.png)  
![SSR](./assets/results/SSR2.png)  

### Reflective Shadow Maps 
![RSM](./assets/results/RSM.png)  

## Physically Based Rendering

### Cook-Torrance BRDF PBR
![Cook-Torrance BRDF](./assets/results/Cook-Torrance%20BRDF.gif)

### LTC Area Light Source
![LTC Area Light Source](./assets/results/ltc-area_light.png)

### Non Photorealistic Rendering
#### Toon
![Toon](./assets/results/toon-style.gif)  
#### Gooch
![Gooch](./assets/results/gooch-style.gif)  
#### Sobel
![Sobel](./assets/results/sobel.gif)  
#### Hatching
![Hatching](./assets/results/hatching.gif)  

## Ray Tracing
### Path Tracing
![Path-Tracing](./assets/results/path_tracing.png)  
#### KVH 
![BVH](./assets/results/BVH.png)  

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
[AKG4e3's Ray Tracing Tutorial-EzRT](https://github.com/AKGWSB/EzRT?tab=readme-ov-file#part-3-opengl-ray-tracing)  
