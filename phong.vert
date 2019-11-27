#version 330

uniform mat4 modelview;
uniform mat4 projection;
uniform vec4 lightPosition;

in  vec3 in_Position;
in  vec3 in_Normal;
out vec3 screenNormal;
out vec3 viewVec;
out vec3 lightVec;

in vec2 in_TexCoord;
out vec2 ex_TexCoord;

out float ex_Dist;
																								
void main(void)
{
	vec4 vertexPosition = modelview * vec4(in_Position,1.0);	
	
	viewVec = normalize(-vertexPosition).xyz;				

	mat3 normalmatrix = transpose(inverse(mat3(modelview)));										
	screenNormal = normalize(normalmatrix * in_Normal);
		
	lightVec = normalize(lightPosition.xyz - vertexPosition.xyz);			
	ex_Dist = distance(vertexPosition, lightPosition);

	ex_TexCoord = in_TexCoord;

    gl_Position = projection * vertexPosition;	
}














