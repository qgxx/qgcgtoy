#version 330 core
layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gFlux;

in vec3 FragCoords;
in vec3 Normal;
in vec3 kd;
in vec3 ks;
in vec3 ka;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{   
    
    vec3 lightColor = vec3(1.0);

    vec3 wo = normalize(viewPos-FragCoords);
    vec3 wi = normalize(lightPos-FragCoords);
    vec3 h  = normalize(wi+wo);
    vec3 normal = normalize(Normal);

    gNormal = normal;
    gPosition = FragCoords;

    vec3 diffuse = kd*max(dot(normal,wi),0)*lightColor;

    vec3 ambident = ka*lightColor*0.3;

    vec3 specular = ks*max(pow(dot(normal,h),64),0);
    gFlux = diffuse;
    
}