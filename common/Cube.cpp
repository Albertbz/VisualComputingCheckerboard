#include "Cube.hpp"

// default cube
// default cube
Cube::Cube() { init(); }
Cube::~Cube() {  // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &colorbuffer);
}
void Cube::init() {
    // 36 vertices (6 faces * 2 triangles * 3 vertices) each with x,y,z
    const GLfloat vertices[108] = {
        // Front face
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
        0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f,

        // Back face
        -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,

        // Left face
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

        // Right face
        0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,

        // Top face
        -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f,

        // Bottom face
        -0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,
        0.5f, -0.5f, -0.5f, 0.5f, -0.5f, -0.5f, -0.5f};

    // Copy into member array
    for (int i = 0; i < 108; ++i) {
        g_vertex_buffer_data[i] = vertices[i];
    }

    // Define per-face colors (6 faces). We'll use simple solid colors per face.
    const GLfloat faceColors[6][3] = {
        {1.0f, 0.0f, 0.0f},  // Front - red
        {0.0f, 1.0f, 0.0f},  // Back - green
        {0.0f, 0.0f, 1.0f},  // Left - blue
        {1.0f, 1.0f, 0.0f},  // Right - yellow
        {1.0f, 0.0f, 1.0f},  // Top - magenta
        {0.0f, 1.0f, 1.0f}   // Bottom - cyan
    };

    // Fill g_color_buffer_data: 6 vertices per face, each vertex gets the face
    // color
    for (int face = 0; face < 6; ++face) {
        for (int v = 0; v < 6; ++v) {
            int idx = (face * 6 + v) * 3;
            g_color_buffer_data[idx + 0] = faceColors[face][0];
            g_color_buffer_data[idx + 1] = faceColors[face][1];
            g_color_buffer_data[idx + 2] = faceColors[face][2];
        }
    }

    // Vertex buffer
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data),
                 g_vertex_buffer_data, GL_STATIC_DRAW);

    // Color buffer
    glGenBuffers(1, &colorbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data),
                 g_color_buffer_data, GL_STATIC_DRAW);
}
void Cube::render(Camera* camera) {
    bindShaders();
    // Build the model matrix - get from object
    glm::mat4 ModelMatrix = this->getTransform();
    glm::mat4 MVP = camera->getViewProjectionMatrix() * ModelMatrix;
    glm::mat4 V = camera->getViewMatrix();
    glm::mat4 P = camera->getProjectionMatrix();
    // Send our transformation to the currently bound shader,
    // in the "MVP" uniform
    shader->updateMatrices(MVP, ModelMatrix, V, P);

    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(0,  // attribute 0. Must match layout in shader.
                          3,  // size
                          GL_FLOAT,   // type
                          GL_FALSE,   // normalized?
                          0,          // stride
                          (void*)0);  // array buffer offset

    // Enable vertex color attribute at location 1
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    glVertexAttribPointer(1,  // attribute 1: color
                          3,  // r,g,b
                          GL_FLOAT, GL_FALSE, 0, (void*)0);

    // Draw the cube (36 vertices)
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}
void Cube::setTransform(glm::mat4 mat) {
    // Set the cube's transformation matrix
    // by changing the object's transform
    this->addTransform(mat - this->getTransform());
}