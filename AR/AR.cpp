// #define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>

// Dear ImGui (vendored under external/imgui/)
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Helper to load shader source from file
static std::string loadShaderSource(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Failed to open shader file: " << path << "\n";
    return std::string();
  }
  std::stringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

// Compile Shader Helper
GLuint compileShader(GLenum type, const char *src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "Shader compile error: " << infoLog << "\n";
  }
  return shader;
}

// Create Shader Program
GLuint createShaderProgram(const char *vertexSrc, const char *fragmentSrc) {
  GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSrc);
  GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
  GLuint program = glCreateProgram();
  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);

  // Check for linking errors
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Program link error: " << infoLog << "\n";
  }

  glDeleteShader(vertex);
  glDeleteShader(fragment);
  return program;
}

// Create Shader Program with optional geometry shader
GLuint createShaderProgramWithGeom(const char *vertexSrc, const char *geomSrc,
                                   const char *fragmentSrc) {
  GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSrc);
  GLuint geom = 0;
  if (geomSrc && geomSrc[0] != '\0') {
    geom = compileShader(GL_GEOMETRY_SHADER, geomSrc);
  }
  GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

  GLuint program = glCreateProgram();
  glAttachShader(program, vertex);
  if (geom)
    glAttachShader(program, geom);
  glAttachShader(program, fragment);
  glLinkProgram(program);

  // Check for linking errors
  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Program link error: " << infoLog << "\n";
  }

  glDeleteShader(vertex);
  if (geom)
    glDeleteShader(geom);
  glDeleteShader(fragment);
  return program;
}

int main() {
  // --- Initialize GLFW ---
  if (!glfwInit())
    return -1;
  // // Add windowHints
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window =
      glfwCreateWindow(1280, 720, "AR Window", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // --- Initialize GLAD ---
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glEnable(GL_DEPTH_TEST);

  // --- Setup Dear ImGui context ---
  const char *glsl_version = "#version 330 core";
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // GUI-controlled parameters
  glm::vec3 guiLightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
  glm::vec3 guiBaseColor = glm::vec3(0.8f, 0.8f, 0.8f);

  // --- OpenCV Capture ---
  cv::VideoCapture cap(0);
  if (!cap.isOpened()) {
    std::cerr << "Cannot open camera\n";
    return -1;
  }

  int frameWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
  int frameHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

  // --- OpenGL Texture ---
  GLuint textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameWidth, frameHeight, 0, GL_RGB,
               GL_UNSIGNED_BYTE, nullptr);

  // --- Quad Vertex Data ---
  float vertices[] = {
      // positions   // tex coords
      -1.f, 1.f,  0.f, 1.f, // top-left
      -1.f, -1.f, 0.f, 0.f, // bottom-left
      1.f,  -1.f, 1.f, 0.f, // bottom-right
      1.f,  1.f,  1.f, 1.f  // top-right
  };
  unsigned int indices[] = {0, 1, 2, 0, 2, 3};

  GLuint VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // texcoord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0); // Unbind the quad's VAO

  // --- Shader Program (loaded from files) ---
  std::string screenVertSrc =
      loadShaderSource("shaders/screenVertexShader.vert");
  std::string screenFragSrc =
      loadShaderSource("shaders/screenFragmentShader.frag");
  if (screenVertSrc.empty() || screenFragSrc.empty()) {
    std::cerr << "Failed to load screen shaders.\n";
    return -1;
  }
  GLuint shaderProgram =
      createShaderProgram(screenVertSrc.c_str(), screenFragSrc.c_str());
  glUseProgram(shaderProgram);
  glUniform1i(glGetUniformLocation(shaderProgram, "frameTex"), 0);

  // --- Cube Vertex Data (duplicate vertices per face, include normals) ---
  // 24 vertices: 6 faces * 4 vertices per face. Each vertex: position (3),
  // normal (3)
  float cubeVertices[] = {
      // Front face (z = +0.5)  normal (0,0,1)
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
      0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f,
      // Back face (z = -0.5) normal (0,0,-1)
      0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
      -1.0f, -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.5f, 0.5f, -0.5f, 0.0f,
      0.0f, -1.0f,
      // Left face (x = -0.5) normal (-1,0,0)
      -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, -0.5f, -0.5f, 0.5f, -1.0f, 0.0f,
      0.0f, -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, -1.0f,
      0.0f, 0.0f,
      // Right face (x = +0.5) normal (1,0,0)
      0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
      0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
      // Top face (y = +0.5) normal (0,1,0)
      -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
      0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
      // Bottom face (y = -0.5) normal (0,-1,0)
      -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.5f, -0.5f, -0.5f, 0.0f, -1.0f,
      0.0f, 0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, -0.5f, -0.5f, 0.5f, 0.0f,
      -1.0f, 0.0f};

  unsigned int cubeIndices[] = {
      0,  1,  2,  0,  2,  3,  // front
      4,  5,  6,  4,  6,  7,  // back
      8,  9,  10, 8,  10, 11, // left
      12, 13, 14, 12, 14, 15, // right
      16, 17, 18, 16, 18, 19, // top
      20, 21, 22, 20, 22, 23  // bottom
  };

  GLuint cubeVAO, cubeVBO, cubeEBO;
  glGenVertexArrays(1, &cubeVAO);
  glGenBuffers(1, &cubeVBO);
  glGenBuffers(1, &cubeEBO);

  glBindVertexArray(cubeVAO);

  glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices,
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices,
               GL_STATIC_DRAW);

  // position attribute (location = 0)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);
  // normal attribute (location = 1)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0); // Unbind the cube's VAO

  // --- Sphere mesh for light marker (UV sphere) ---
  GLuint sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
  int sphereIndexCount = 0;
  {
    const int latSegments = 16;
    const int lonSegments = 32;
    std::vector<float> sphereVerts;
    std::vector<unsigned int> sphereIndices;
    sphereVerts.reserve((latSegments + 1) * (lonSegments + 1) * 3);
    sphereIndices.reserve(latSegments * lonSegments * 6);

    for (int y = 0; y <= latSegments; ++y) {
      float v = (float)y / (float)latSegments;
      float theta = v * glm::pi<float>();
      for (int x = 0; x <= lonSegments; ++x) {
        float u = (float)x / (float)lonSegments;
        float phi = u * glm::two_pi<float>();
        float xs = sin(theta) * cos(phi);
        float ys = cos(theta);
        float zs = sin(theta) * sin(phi);
        sphereVerts.push_back(xs);
        sphereVerts.push_back(ys);
        sphereVerts.push_back(zs);
      }
    }

    for (int y = 0; y < latSegments; ++y) {
      for (int x = 0; x < lonSegments; ++x) {
        int first = y * (lonSegments + 1) + x;
        int second = first + lonSegments + 1;
        sphereIndices.push_back(first);
        sphereIndices.push_back(second);
        sphereIndices.push_back(first + 1);

        sphereIndices.push_back(second);
        sphereIndices.push_back(second + 1);
        sphereIndices.push_back(first + 1);
      }
    }

    sphereIndexCount = static_cast<int>(sphereIndices.size());

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVerts.size() * sizeof(float),
                 sphereVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sphereIndices.size() * sizeof(unsigned int),
                 sphereIndices.data(), GL_STATIC_DRAW);

    // position attribute only (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
  }

  // --- Cube Shader Program (loaded from files) ---
  std::string cubeVertSrc = loadShaderSource("shaders/cubeVertexShader.vert");
  std::string cubeFragSrc = loadShaderSource("shaders/cubeFragmentShader.frag");
  if (cubeVertSrc.empty() || cubeFragSrc.empty()) {
    std::cerr << "Failed to load cube shaders.\n";
    return -1;
  }
  GLuint cubeShaderProgram =
      createShaderProgram(cubeVertSrc.c_str(), cubeFragSrc.c_str());

  // --- Unlit shader for rendering the light marker (simple solid color) ---
  std::string unlitVertSrc = loadShaderSource("shaders/unlitVertexShader.vert");
  std::string unlitFragSrc =
      loadShaderSource("shaders/unlitFragmentShader.frag");
  if (unlitVertSrc.empty() || unlitFragSrc.empty()) {
    std::cerr << "Failed to load unlit shaders.\n";
    return -1;
  }
  GLuint unlitProgram =
      createShaderProgram(unlitVertSrc.c_str(), unlitFragSrc.c_str());

  // --- Define camera matrix and distortion coefficients ---
  cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << 2218.397864043568, 0.,
                          959.5, 0., 2218.397864043568, 539.5, 0., 0., 1.);
  cv::Mat distCoeffs = (cv::Mat_<double>(5, 1) << -0.17611576780242291,
                        1.7357972971751359, 0., 0., -5.4837634455342661);

  // --- CALCULATE PROJECTION MATRIX USING CAMERA INTRINSICS ---
  float nearPlane = 0.01f;
  float farPlane = 100.0f;
  float fx = cameraMatrix.at<double>(0, 0);
  float fy = cameraMatrix.at<double>(1, 1);
  float cx = cameraMatrix.at<double>(0, 2);
  float cy = cameraMatrix.at<double>(1, 2);

  glm::mat4 projection = glm::mat4(0.0f);
  projection[0][0] = 2.0f * fx / frameWidth;
  projection[1][1] = 2.0f * fy / frameHeight;
  projection[2][0] = 1.0f - (2.0f * cx) / frameWidth;
  projection[2][1] = (2.0f * cy) / frameHeight -
                     1.0f; // Flipped for OpenGL's Y-up coord system
  projection[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
  projection[2][3] = -1.0f;
  projection[3][2] = -2.0f * farPlane * nearPlane / (farPlane - nearPlane);

  // --- Main Loop ---
  while (!glfwWindowShouldClose(window)) {
    cv::Mat frame;
    cap >> frame;
    if (frame.empty())
      break;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // GUI: place window in upper-right corner
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_AlwaysAutoResize |
                                    ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always,
                            ImVec2(0.0f, 0.0f));
    ImGui::Begin("Lighting", nullptr, window_flags);
    {
      float lightArr[3] = {guiLightDir.x, guiLightDir.y, guiLightDir.z};
      ImGui::SliderFloat3("Light Dir", lightArr, -1.0f, 1.0f);
      guiLightDir =
          glm::normalize(glm::vec3(lightArr[0], lightArr[1], lightArr[2]));

      float baseCol[3] = {guiBaseColor.r, guiBaseColor.g, guiBaseColor.b};
      ImGui::ColorEdit3("Base Color", baseCol);
      guiBaseColor = glm::vec3(baseCol[0], baseCol[1], baseCol[2]);
    }
    ImGui::End();

    // 1. Create a grayscale copy of the ORIGINAL frame for detection
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // 2. Find checkerboard corners in the original, un-flipped image
    std::vector<cv::Point2f> corners;
    bool found = cv::findChessboardCorners(gray, cv::Size(9, 6), corners,
                                           cv::CALIB_CB_ADAPTIVE_THRESH |
                                               cv::CALIB_CB_NORMALIZE_IMAGE |
                                               cv::CALIB_CB_FAST_CHECK);

    // 3. Convert the color frame to RGB for drawing
    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);

    cv::Mat rvec, tvec; // Declare here to be in scope for cube rendering

    if (found) {
      cv::cornerSubPix(
          gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
          cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30,
                           0.1));

      std::vector<cv::Point3f> objectPoints;
      for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 9; ++j) {
          objectPoints.push_back(cv::Point3f(j * 0.025f, i * 0.025f, 0));
        }
      }

      // 4. Get correct pose data from the un-flipped corners
      cv::solvePnP(objectPoints, corners, cameraMatrix, distCoeffs, rvec, tvec);

      // Draw axes for visualization on the un-flipped color frame
      // std::vector<cv::Point3f> axisPoints;
      // axisPoints.push_back(cv::Point3f(0, 0, 0));
      // axisPoints.push_back(cv::Point3f(0.075f, 0, 0));
      // axisPoints.push_back(cv::Point3f(0, 0.075f, 0));
      // axisPoints.push_back(cv::Point3f(0, 0, -0.075f));
      // std::vector<cv::Point2f> imagePoints;
      // cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix, distCoeffs,
      //                   imagePoints);
      // cv::line(frame, imagePoints[0], imagePoints[1], cv::Scalar(255, 0, 0),
      // 5); cv::line(frame, imagePoints[0], imagePoints[2], cv::Scalar(0, 255,
      // 0), 5); cv::line(frame, imagePoints[0], imagePoints[3], cv::Scalar(0,
      // 0, 255), 5);
    }

    // 5. Flip the final frame (with or without axes) vertically for OpenGL
    cv::flip(frame, frame, 0);

    // --- RENDER EVERYTHING ---

    // Upload the prepared frame to the OpenGL texture
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGB,
                    GL_UNSIGNED_BYTE, frame.data);

    // Clear buffers and render the video background (happens every frame)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthMask(GL_FALSE); // Disable depth writing for background
    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE); // Re-enable depth writing for 3D objects

    // If found, render the cube on top
    if (found) {
      glUseProgram(cubeShaderProgram);

      glm::mat4 view = glm::mat4(1.0f);

      cv::Mat R;
      cv::Rodrigues(rvec, R);
      glm::mat4 model = glm::mat4(1.0f);
      for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
          model[i][j] = R.at<double>(j, i);
        }
      }
      model[3][0] = tvec.at<double>(0, 0);
      model[3][1] = tvec.at<double>(1, 0);
      model[3][2] = tvec.at<double>(2, 0);

      float scale = 0.025f;

      // Translate the model so that the cube has its corner at the origin
      // (using half the cube size since we scale it down)
      model = glm::translate(
          model, glm::vec3(scale / 2.0f, scale / 2.0f, -scale / 2.0f));

      // Translate an additional amount to center the cube on the checkerboard
      model = glm::translate(model, glm::vec3(0.1f, 0.05f, 0.0f));

      // Scale down the cube
      model = glm::scale(model, glm::vec3(scale));

      glm::mat4 axisCorrection =
          glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, -1.0f));
      model = axisCorrection * model;

      glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "projection"),
                         1, GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "view"), 1,
                         GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(cubeShaderProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));

      // Compute and upload normal matrix (inverse-transpose of model's
      // upper-left 3x3)
      glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
      glUniformMatrix3fv(
          glGetUniformLocation(cubeShaderProgram, "normalMatrix"), 1, GL_FALSE,
          glm::value_ptr(normalMatrix));

      // Light and material uniforms (driven by GUI)
      // Transform the GUI light direction into camera space using the
      // final model's linear part so it is expressed in the same space as
      // the transformed normals. This uses the full `model` (including
      // axisCorrection/scale) rather than the raw Rodrigues rotation `R`.
      glm::vec3 lightDirCam = glm::normalize(glm::mat3(model) * guiLightDir);
      glUniform3fv(glGetUniformLocation(cubeShaderProgram, "lightDir"), 1,
                   glm::value_ptr(lightDirCam));
      glUniform3fv(glGetUniformLocation(cubeShaderProgram, "baseColor"), 1,
                   glm::value_ptr(guiBaseColor));

      // Draw cube
      glBindVertexArray(cubeVAO);
      glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

      // --- Render light marker (small solid yellow cube) ---
      // Compute cube center in camera space from the model matrix
      glm::vec3 cubePos(model[3][0], model[3][1], model[3][2]);
      // Transform the GUI light direction into camera space (same as for
      // shading)
      glm::vec3 markerDir = glm::normalize(glm::mat3(model) * guiLightDir);
      // Distance from cube to marker (in meters). 0.10 = 10 cm
      float markerDistance = 0.10f;
      glm::vec3 markerPos = cubePos + markerDir * markerDistance;

      // Build marker model matrix (positioned in camera space)
      glm::mat4 markerModel = glm::mat4(1.0f);
      markerModel = glm::translate(markerModel, markerPos);
      // Small marker size (1 cm cube)
      float markerScale = 0.01f;
      markerModel = glm::scale(markerModel, glm::vec3(markerScale));

      // Draw the marker with the unlit shader so it appears fully lit
      glUseProgram(unlitProgram);
      glUniformMatrix4fv(glGetUniformLocation(unlitProgram, "projection"), 1,
                         GL_FALSE, glm::value_ptr(projection));
      glUniformMatrix4fv(glGetUniformLocation(unlitProgram, "view"), 1,
                         GL_FALSE, glm::value_ptr(view));
      glUniformMatrix4fv(glGetUniformLocation(unlitProgram, "model"), 1,
                         GL_FALSE, glm::value_ptr(markerModel));
      glm::vec3 yellow(1.0f, 1.0f, 0.0f);
      glUniform3fv(glGetUniformLocation(unlitProgram, "color"), 1,
                   glm::value_ptr(yellow));

      // Draw sphere marker (unlit)
      glBindVertexArray(sphereVAO);
      glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
    }

    // Render ImGui on top
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // Cleanup ImGui
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shaderProgram);

  glDeleteVertexArrays(1, &cubeVAO);
  glDeleteBuffers(1, &cubeVBO);
  glDeleteBuffers(1, &cubeEBO);
  glDeleteProgram(cubeShaderProgram);
  glDeleteProgram(unlitProgram);
  // Delete sphere buffers
  glDeleteVertexArrays(1, &sphereVAO);
  glDeleteBuffers(1, &sphereVBO);
  glDeleteBuffers(1, &sphereEBO);

  glfwTerminate();
  return 0;
}
