#version 330 core

// Phong related variables
uniform sampler2D texture_diffuse0;
uniform vec3 uKd;
uniform vec3 uKs;
uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform vec3 uLightIntensity;
uniform int uShadowClass;

in vec2 vTextureCoord;
in vec3 vFragPos;
in vec3 vNormal;

// Shadow map related variables
#define NUM_SAMPLES 50
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define NUM_RINGS 10

#define SHADOW_MAP_SIZE 2048.
#define FILTER_RADIUS 10.
#define FRUSTUM_SIZE 400.
#define NEAR_PLANE 0.01
#define LIGHT_WORLD_SIZE 5.
#define LIGHT_SIZE_UV LIGHT_WORLD_SIZE / FRUSTUM_SIZE

#define EPS 1e-3
#define PI 3.141592653589793
#define PI2 6.283185307179586

uniform sampler2D uShadowMap;

in vec4 vPositionFromLight;

float rand_1to1(float x ) { 
    // -1 -1
    return fract(sin(x)*10000.0);
}

float rand_2to1(vec2 uv ) { 
  	// 0 - 1
	const float a = 12.9898, b = 78.233, c = 43758.5453;
	float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

float unpack(vec4 rgbaDepth) {
    const vec4 bitShift = vec4(1.0, 1.0/256.0, 1.0/(256.0*256.0), 1.0/(256.0*256.0*256.0));
    return dot(rgbaDepth, bitShift);
}

vec2 poissonDisk[NUM_SAMPLES];

void poissonDiskSamples( const in vec2 randomSeed ) {

	float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
	float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

	float angle = rand_2to1( randomSeed ) * PI2;
	float radius = INV_NUM_SAMPLES;
	float radiusStep = radius;

	for( int i = 0; i < NUM_SAMPLES; i ++ ) {
		poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
		radius += radiusStep;
		angle += ANGLE_STEP;
	}
}

void uniformDiskSamples( const in vec2 randomSeed ) {

	float randNum = rand_2to1(randomSeed);
	float sampleX = rand_1to1( randNum ) ;
	float sampleY = rand_1to1( sampleX ) ;

	float angle = sampleX * PI2;
	float radius = sqrt(sampleY);

	for( int i = 0; i < NUM_SAMPLES; i ++ ) {
		poissonDisk[i] = vec2( radius * cos(angle) , radius * sin(angle)  );

		sampleX = rand_1to1( sampleY ) ;
		sampleY = rand_1to1( sampleX ) ;

		angle = sampleX * PI2;
		radius = sqrt(sampleY);
	}
}

// Adaptively Shadow Bias
float getShadowBias(float c, float filterRadiusUV){
	vec3 normal = normalize(vNormal);
	vec3 lightDir = normalize(uLightPos - vFragPos);
	float fragSize = (1. + ceil(filterRadiusUV)) * (FRUSTUM_SIZE / SHADOW_MAP_SIZE / 2.);
	return max(fragSize, fragSize * (1.0 - dot(normal, lightDir))) * c;
}

float useShadowMap(sampler2D shadowMap, vec4 shadowCoord, float biasC, float filterRadiusUV){
	float depth = unpack(texture2D(shadowMap, shadowCoord.xy));
	float cur_depth = shadowCoord.z;
	float bias = getShadowBias(biasC, filterRadiusUV);
	if(cur_depth - bias >= depth + EPS){
		return 0.;
	}
	else{
		return 1.0;
	}
}

float PCF(sampler2D shadowMap, vec4 coords, float biasC, float filterRadiusUV) {
	//uniformDiskSamples(coords.xy);
	poissonDiskSamples(coords.xy); //使用xy坐标作为随机种子生成
	float visibility = 0.0;
	for(int i = 0; i < NUM_SAMPLES; i++){
		vec2 offset = poissonDisk[i] * filterRadiusUV;
		float shadowDepth = useShadowMap(shadowMap, coords + vec4(offset, 0., 0.), biasC, filterRadiusUV);
		if(coords.z > shadowDepth + EPS){
			visibility++;
		}
	}
	return 1.0 - visibility / float(NUM_SAMPLES);
}

float findBlocker(sampler2D shadowMap, vec2 uv, float zReceiver) {
	int blockerNum = 0;
	float blockDepth = 0.;

	float posZFromLight = vPositionFromLight.z;

	float searchRadius = LIGHT_SIZE_UV * (posZFromLight - NEAR_PLANE) / posZFromLight;

	poissonDiskSamples(uv);
	for(int i = 0; i < NUM_SAMPLES; i++){
		float shadowDepth = unpack(texture2D(shadowMap, uv + poissonDisk[i] * searchRadius));
		if(zReceiver > shadowDepth){
			blockerNum++;
			blockDepth += shadowDepth;
		}
	}

	if(blockerNum == 0)
		return -1.;
	else
		return blockDepth / float(blockerNum);
}

float PCSS(sampler2D shadowMap, vec4 coords, float biasC){
	float zReceiver = coords.z;

	float avgBlockerDepth = findBlocker(shadowMap, coords.xy, zReceiver);

	if(avgBlockerDepth < -EPS)
		return 1.0;

	float penumbra = (zReceiver - avgBlockerDepth) * LIGHT_SIZE_UV / avgBlockerDepth;
	float filterRadiusUV = penumbra;

	return PCF(shadowMap, coords, biasC, filterRadiusUV);
}

vec3 blinnPhong() {
	vec3 color = texture2D(texture_diffuse0, vTextureCoord).rgb;
	color = pow(color, vec3(2.2));

	vec3 ambient = 0.05 * color;

	vec3 lightDir = normalize(uLightPos);
	vec3 normal = normalize(vNormal);
	float diff = max(dot(lightDir, normal), 0.0);
	vec3 light_atten_coff =
		uLightIntensity / pow(length(uLightPos - vFragPos), 2.0);
	vec3 diffuse = diff * light_atten_coff * color;

	vec3 viewDir = normalize(uCameraPos - vFragPos);
	vec3 halfDir = normalize((lightDir + viewDir));
	float spec = pow(max(dot(halfDir, normal), 0.0), 32.0);
	vec3 specular = uKs * light_atten_coff * spec;

	vec3 radiance = (ambient + diffuse + specular);
	vec3 phongColor = pow(radiance, vec3(1.0 / 2.2));
	return phongColor;
}

void main(void) {
	vec3 shadowCoord = vPositionFromLight.xyz / vPositionFromLight.w;
	shadowCoord.xyz = (shadowCoord.xyz + 1.0) / 2.0;

	float visibility = 1.;

	float nonePCFBiasC = .4;
	float pcfBiasC = .08;
	float filterRadiusUV = FILTER_RADIUS / SHADOW_MAP_SIZE;

	if (uShadowClass == 0) {
		visibility = useShadowMap(uShadowMap, vec4(shadowCoord, 1.0), nonePCFBiasC, 0.);
	}
	else if (uShadowClass == 1) {
		visibility = PCF(uShadowMap, vec4(shadowCoord, 1.0), pcfBiasC, filterRadiusUV);
	}
	else if (uShadowClass == 2) {
		visibility = PCSS(uShadowMap, vec4(shadowCoord, 1.0), pcfBiasC);
	}

	vec3 phongColor = blinnPhong();

	gl_FragColor = vec4(phongColor * visibility, 1.0);
}