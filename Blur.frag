#version 330
precision highp float;

uniform float val0;
uniform float val1;
uniform float val2;
uniform float val3;
uniform float val4;
uniform float val5;

uniform sampler2D textureUnit0;
uniform sampler2D textureUnit1;
uniform sampler2D textureUnit2;
uniform sampler2D textureUnit3;
uniform sampler2D textureUnit4;
uniform sampler2D textureUnit5;

in vec2 ex_TexCoord;
layout(location = 0) out vec4 out_Color;
 
void main(void) 
{
	vec4 tex0 = texture(textureUnit0, ex_TexCoord);
	vec4 tex1 = texture(textureUnit1, ex_TexCoord);
	vec4 tex2 = texture(textureUnit2, ex_TexCoord);
	vec4 tex3 = texture(textureUnit3, ex_TexCoord);
	vec4 tex4 = texture(textureUnit4, ex_TexCoord);
	vec4 tex5 = texture(textureUnit5, ex_TexCoord);

	vec4 texWeight0 = tex0 * val0;				
	vec4 texWeight1 = tex1 * val1;				
	vec4 texWeight2 = tex2 * val2;				
	vec4 texWeight3 = tex3 * val3;				
	vec4 texWeight4 = tex4 * val4;				
	vec4 texWeight5 = tex5 * val5;				

	out_Color = (texWeight0 + texWeight1 + texWeight2 + texWeight3 + texWeight4 + texWeight5) / 6;
}

