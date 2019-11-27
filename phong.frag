#version 330
precision highp float;

struct lightStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
};

struct materialStruct
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform lightStruct light;
uniform materialStruct material;
uniform sampler2D textureUnit0;

uniform float attConst;
uniform float attLinear;
uniform float attQuadratic;

in vec3 screenNormal;
in vec3 viewVec;
in vec3 lightVec;
in vec2 ex_TexCoord;
in float ex_Dist;
layout(location = 0) out vec4 out_Color;
 
void main(void) 
{
	vec4 ambientI = light.ambient * material.ambient;											

	
	vec4 diffuseI = light.diffuse * material.diffuse;											
	diffuseI = diffuseI * max(dot(normalize(screenNormal),normalize(lightVec)),0);
	
	vec3 reflectedVec = normalize(reflect(normalize(-lightVec),normalize(screenNormal)));								

	vec4 specularI = light.specular * material.specular;
	specularI = specularI * pow(max(dot(reflectedVec,viewVec),0), material.shininess);

	float attenuation = 1.0f / (attConst + attLinear * ex_Dist + attQuadratic * ex_Dist * ex_Dist);

	vec4 tmp_color =  diffuseI + specularI;

	vec4 litColor = ambientI + vec4(tmp_color.rgb * attenuation, 1.0);

	out_Color = litColor * texture2D(textureUnit0, ex_TexCoord);
}