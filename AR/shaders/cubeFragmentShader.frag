#version 330 core

in vec3 vNormal;
in vec3 vFragPos;
out vec4 FragColor;

uniform vec3 lightDir; // direction of incoming light (should be normalized)
uniform vec3 baseColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightDir);
    float diff = max(dot(N, L), 0.0);
    float ambient = 0.2;
    float diffuseFactor = 0.8;
    vec3 color = baseColor * (ambient + diffuseFactor * diff);
    FragColor = vec4(color, 1.0);
}