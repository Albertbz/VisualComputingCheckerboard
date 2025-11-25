#version 330 core

out vec4 FragColor;

// Color is set from the application via the uniform named "colorValue"
uniform vec4 colorValue;

void main() {
    FragColor = colorValue;
}