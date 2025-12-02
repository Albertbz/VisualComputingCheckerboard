#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

out vec3 vNormal;
out vec3 vFragPos;

void main()
{
    // Position in world/camera space (model places object in camera coords)
    vFragPos = vec3(model * vec4(aPos, 1.0));
    // Transform normal with the provided normal matrix
    vNormal = normalMatrix * aNormal;
    gl_Position = projection * view * vec4(vFragPos, 1.0);
}