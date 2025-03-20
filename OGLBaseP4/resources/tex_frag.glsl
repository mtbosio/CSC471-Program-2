#version 330 core
uniform sampler2D Texture0;
uniform float MatShine;
uniform int flip;

in vec2 vTexCoord;
in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;


out vec4 Outcolor;


void main() {
	vec3 normal = normalize(flip == 1 ? fragNor : -fragNor);

	vec3 light = normalize(lightDir);
	float dC = max(0.0, dot(normal, light));

	vec4 texColor0 = texture(Texture0, vTexCoord);

	vec3 halfV = normalize(-1*EPos) + normalize(light);
	float sC = pow(max(dot(normalize(halfV), normal), 0), MatShine);
	
	Outcolor = vec4(texColor0.xyz * 0.1 + dC*texColor0.xyz + sC * texColor0.xyz, 1.0);

}
