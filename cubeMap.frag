#version 330
precision highp float;

out vec4 outColour;

smooth in vec3 cubeTexCoord;
uniform samplerCube cubeMap;
 
void main(void) 
{   
	outColour = texture(cubeMap, cubeTexCoord);
}