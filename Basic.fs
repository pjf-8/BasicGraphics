#version 430 core
// A7 4/26

layout(location=0) out vec4 out_color;

in vec4 vertexColor; // Now interpolated across face

in vec4 interPos;
in vec3 interNormal;
struct PointLight {
vec4 pos;
vec4 color;
};
uniform PointLight light;
uniform float metallic;
uniform float roughness;
const float PI = 3.14159265359;
//A8
in vec2 interUV;
in vec3 interTangent;
uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;

// A7 Function getFresnelAtAngleZero

vec3 getFresnelAtAngleZero(vec3 albedo, float metallic){
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	return F0;
}

// A7 Function getFresnel

vec3 getFresnel(vec3 F0, vec3 L, vec3 H){
	float cosAngle = max(0, dot(L , H));
	vec3 schlick_val = F0 + (1 - F0) * (pow(1 - max(0, cosAngle), 5));
	return schlick_val;
}

// A7 Function getNDF PBR45

float getNDF(vec3 H, vec3 N, float roughness){
	float a = pow(roughness, 2);
	float ndf = pow(a,2) / (PI*pow(pow(dot(N,H),2) * (pow(a,2)-1)+1,2));
	return ndf;
}

// A7 Function getSchlickGeo

float getSchlickGeo(vec3 B, vec3 N, float roughness){
	float k = (pow(roughness, 2) / 8);
	float geo_val = (dot(N,B)) / (dot(N,B) * (1-k) + k);
	return geo_val;
}

// A7 getGF

float getGF(vec3 L, vec3 V, vec3 N, float roughness){
	float GL = getSchlickGeo(L, N, roughness);
	float GV = getSchlickGeo(V, N, roughness);
	return GL * GV;
}

void main()
{
	vec3 N = normalize(interNormal);
	vec3 L = vec3(light.pos - interPos);
  L = normalize(L); // Fixed from comment
  // A8
  	vec3 T = normalize(interTangent);
	T = normalize(T - dot(T,N)*N);
    vec3 B = normalize(cross(N,T));
    vec3 texN = vec3(texture(normalTexture, interUV));
    texN.x = texN.x*2.0f - 1.0f;
    texN.y = texN.y*2.0f - 1.0f;
    texN = normalize(texN);
    mat3 toView = mat3(T,B,N);
    N = normalize(toView*texN);
	// A8
	float diffuseCo = max(0, dot(L, N));
	vec3 diffColor = diffuseCo * vec3(light.color * vertexColor);
	vec3 texColor = vec3(texture(diffuseTexture, interUV)); // A8
  // A7 Block 1 normalize and set values
  vec3 V = normalize(-vec3(interPos));
  vec3 F0 = getFresnelAtAngleZero(vec3(texColor), metallic);
  vec3 H = normalize(V + L);
  vec3 F = getFresnel(F0, L, H);
  vec3 KS = F;
  // A7 Block 2 Calculate the complete diffuse color
  vec3 KD = 1.0 - KS;
	KD = KD * (1.0 - metallic);
	KD = KD * vec3(texColor);
	KD = KD / PI;
  // A7 BLock 3 Calculate the complete specular reflection
  float NDF = getNDF(H, N, roughness);
	float G = getGF(L, V, N, roughness);
	KS = KS * NDF * G;
	KS = KS / ((4.0 * max(0, dot(N,L)) * max(0, dot(N,V))) + 0.0001);
  // A7 Block 4 Final and Out color
  vec3 finalColor = (KD + KS)*vec3(light.color)*max(0, dot(N,L));
  out_color = vec4(finalColor, 1.0);

}
