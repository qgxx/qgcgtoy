#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
 
uniform  vec3 ukd;
uniform  vec3 uka;
uniform  vec3 uks; 

uniform mat4 model;
uniform mat4 lightSpace;

out vec3 FragCoords;
out vec3 Normal;
out vec3 kd;
out vec3 ks;
out vec3 ka;

void main()
{   

    Normal =transpose(inverse(mat3(model))) * aNormal;
    vec4 world = model*vec4( aPos, 1.0);
    FragCoords = world.xyz;
    gl_Position = lightSpace*model*vec4( aPos, 1.0);
    kd = ukd;
    ks = uks;
    ka = uka;

}