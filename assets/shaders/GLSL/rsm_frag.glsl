#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec4 LightSpaceCoord;
in vec3 kd;
in vec3 ks;
in vec3 ka;

uniform sampler2D  positionTexture;
uniform sampler2D  normalTexture;
uniform sampler2D  fluxTexture;
uniform sampler2D  depthTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float filterRange;

out vec4 FragColor;
#define SAMPLE_NUMBER 40
layout(std140) uniform randomMap
{
	vec4 randomArray[SAMPLE_NUMBER];
};

float shadow(vec4 LightSpaceCoord) {

	 vec3 shadowCoord = LightSpaceCoord.xyz/LightSpaceCoord.w;
	 shadowCoord = (shadowCoord+1.0)*0.5;
	 float closet = texture(depthTexture,shadowCoord.xy).r;
	 float depth = shadowCoord.z;

	 return depth-0.01<closet ? 1.0:0.0;
}

void main() {
	vec3 lightColor = vec3(1.0);
	vec3 wo = normalize(viewPos-FragPos);
	vec3 wi = normalize(lightPos-FragPos);
	vec3 h  = normalize(wi+wo);
	vec3 normal = normalize(Normal);
	vec3 diffuse = kd*max(dot(normal,wi),0);

	vec3 ambident = ka*lightColor*0.3;

	vec3 specular = ks*max(pow(dot(normal,h),64),0);

	float v = shadow(LightSpaceCoord);

	vec3 dir_color = (ambident+v*(diffuse+specular))*lightColor;

	vec3 indir_color = vec3(1.0);
	vec3 projectCoord = LightSpaceCoord.xyz/LightSpaceCoord.w;
	projectCoord = projectCoord*0.5 + 0.5;

	float filterSize = 1024;

	for(int i=0;i<SAMPLE_NUMBER;i++){
		vec2 uv = projectCoord.xy+randomArray[i].xy*(filterRange/filterSize);
		vec3 irrandance = texture(fluxTexture,uv).rgb;
		vec3 idir_p = texture(positionTexture,uv).rgb;
		vec3 idir_wi = normalize(texture(positionTexture,uv).rgb-FragPos);
		vec3 idir_normal = normalize(texture(normalTexture,uv).rgb);
		vec3 sample_color = (irrandance*max(dot(idir_wi,normal),0)*max(dot(idir_normal,FragPos-idir_p),0)/pow(length(idir_p-FragPos),2))*randomArray[i].z;
		//vec3 sample_color = irrandance;
		indir_color+=sample_color;
	}
	indir_color=clamp(indir_color/SAMPLE_NUMBER, 0.0, 1.0);

	FragColor = vec4(indir_color*20+dir_color,1.0);
	// FragColor = vec4( gl_FragCoord.x/1024);
}