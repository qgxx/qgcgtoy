#version 330 core
in vec2 texCoord;
uniform sampler2D  positionTexture;
uniform sampler2D  normalTexture;
uniform sampler2D  fluxTexture;
uniform sampler2D  depthTexture;
out vec4 FragColor;

void main(){

FragColor = vec4(texture2D( fluxTexture,texCoord));


}