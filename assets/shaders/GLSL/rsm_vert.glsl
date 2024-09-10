#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;

out vec3 Normal;
out vec3 FragPos;
out vec4 LightSpaceCoord;

uniform  vec3 ukd;
uniform  vec3 uka;
uniform  vec3 uks; 
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpace;
out vec3 kd;
out vec3 ks;
out vec3 ka;
void main(){
 Normal = transpose(inverse(mat3(model))) * aNormal;
 FragPos = vec4(model * vec4(aPos,1.0)).xyz;
 LightSpaceCoord = lightSpace * model * vec4(aPos,1.0);
 gl_Position=projection*view* model*vec4(aPos, 1.0f);
    kd = ukd;
    ks = uks;
    ka = uka;
}