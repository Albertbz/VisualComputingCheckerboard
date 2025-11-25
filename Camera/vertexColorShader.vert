#version 330 core

layout (location = 0) in vec3 vertexPosition_modelspace;
layout (location = 1) in vec3 vertexColor;

// Matrices provided by Shader::updateMatrices
uniform mat4 MVP;

out vec3 fragColor;

void main() {
    fragColor = vertexColor;
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1.0);
}
