#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <opencv2/opencv.hpp>
#include <iostream>

// Vertex Shader (pass-through)
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main()
{
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment Shader (display texture)
const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D frameTex;
void main()
{
    FragColor = texture(frameTex, TexCoord);
}
)";

// Compile Shader Helper
GLuint compileShader(GLenum type, const char* src) {
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
GLuint createShaderProgram() {
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

int main() {
    // --- Initialize GLFW ---
    if (!glfwInit()) return -1;
		// // Add windowHints
		glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "OpenCV -> OpenGL Background", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    
    // --- Initialize GLAD ---
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    return -1;
}


    // --- OpenCV Capture ---
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) { std::cerr << "Cannot open camera\n"; return -1; }

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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameWidth, frameHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // --- Quad Vertex Data ---
    float vertices[] = {
        // positions   // tex coords
        -1.f,  1.f,    0.f, 1.f,  // top-left
        -1.f, -1.f,    0.f, 0.f,  // bottom-left
         1.f, -1.f,    1.f, 0.f,  // bottom-right
         1.f,  1.f,    1.f, 1.f   // top-right
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texcoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // --- Shader Program ---
    GLuint shaderProgram = createShaderProgram();
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "frameTex"), 0);

		// --- Define camera matrix and distortion coefficients ---
		cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << 2218.397864043568, 0.,
                            959.5, 0., 2218.397864043568, 539.5, 0., 0., 1.);
    cv::Mat distCoeffs = (cv::Mat_<double>(5, 1) << -0.17611576780242291,
                          1.7357972971751359, 0., 0., -5.4837634455342661);

    // --- Main Loop ---
    while (!glfwWindowShouldClose(window)) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) break;

				// Convert to grayscale for AR marker detection
				
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
				// Flip frame vertically to match OpenGL texture coordinates
				cv::flip(frame, frame, 0);
				cv::Mat gray;
				cv::cvtColor(frame, gray, cv::COLOR_RGB2GRAY);

				// Find checkerboard corners (for AR marker simulation)
				std::vector<cv::Point2f> corners;
				bool found = cv::findChessboardCorners(gray, cv::Size(9,6), corners,
					cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
				if (found) {
					// Draw axes for visualization using the precise corners
					cv::cornerSubPix(gray, corners, cv::Size(11,11), cv::Size(-1,-1),
						cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

					// Find translation and rotation vectors
					std::vector<cv::Point3f> objectPoints;
					for (int i = 0; i < 6; ++i) {
						for (int j = 0; j < 9; ++j) {
							objectPoints.push_back(cv::Point3f(j * 0.025f, i * 0.025f, 0)); // Assuming square size of 2.5 cm
						}
					}

					cv::Mat rvec, tvec;
					cv::solvePnP(objectPoints, corners, cameraMatrix, distCoeffs, rvec, tvec);

					// Draw axes for visualization
					std::vector<cv::Point3f> axisPoints;
					axisPoints.push_back(cv::Point3f(0, 0, 0));
					axisPoints.push_back(cv::Point3f(0.075f, 0, 0));
					axisPoints.push_back(cv::Point3f(0, 0.075f, 0));
					axisPoints.push_back(cv::Point3f(0, 0, -0.075f));
					std::vector<cv::Point2f> imagePoints;
					cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix, distCoeffs, imagePoints);
					cv::line(frame, imagePoints[0], imagePoints[1], cv::Scalar(255,0,0), 5); // X-axis
					cv::line(frame, imagePoints[0], imagePoints[2], cv::Scalar(0,255,0), 5); // Y-axis
					cv::line(frame, imagePoints[0], imagePoints[3], cv::Scalar(0,0,255), 5); // Z-axis
				}

        // Upload frame to OpenGL texture
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_RGB, GL_UNSIGNED_BYTE, frame.data);

        // Render
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
