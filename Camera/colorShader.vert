#version 330 core

layout (location = 0) in vec3 vertexPosition_modelspace;

// Matrices provided by Shader::updateMatrices (uniform names expected by Shader.cpp)
uniform mat4 MVP;

void main() {
    // Transform vertex by the Model-View-Projection matrix
    gl_Position = MVP * vec4(vertexPosition_modelspace, 1.0);
}