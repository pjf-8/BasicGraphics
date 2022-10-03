#version 430 core

layout(location=0) in vec3 position;
layout(location=1) in vec4 color;

out vec4 vertexColor;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
layout(location = 2) in vec3 normal;
uniform mat3 normMat;
out vec4 interPos;
out vec3 interNormal;
// A8
layout(location = 3) in vec2 texcoords;
layout(location = 4) in vec3 tangent;
out vec2 interUV;
out vec3 interTangent;


void main()
{		
	// Get position of vertex (object space)
	vec4 objPos = vec4(position, 1.0);

	//A8
	interUV = texcoords;

	vec4 vpos = viewMat * modelMat * objPos;
    interPos = vpos;

	// For now, just pass along vertex position (no transformations)
	gl_Position = projMat * viewMat * modelMat * objPos;
	interNormal = normMat * normal;

	//A8
	interTangent = vec3(viewMat * modelMat * vec4(tangent, 0));

	// Output per-vertex color
	vertexColor = color;
}
