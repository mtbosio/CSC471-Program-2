#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform float MatShine;
uniform vec3 MatSpec;


//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	float dC = max(0, dot(normal, light));

	vec3 viewDir = normalize(-EPos);
	vec3 halfVector = normalize(viewDir) + normalize(light);
	float sC = pow(max(0, dot(normalize(halfVector), normal)), MatShine);
	color = vec4(MatAmb + (dC * MatDif) + sC * MatSpec, 1.0);
}
