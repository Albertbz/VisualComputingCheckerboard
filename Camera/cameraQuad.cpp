#include <glad/gl.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#define GLAD_GL_IMPLEMENTATION

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/Camera.hpp>
#include <common/ColorShader.hpp>
#include <common/Cube.hpp>
#include <common/Object.hpp>
#include <common/Quad.hpp>
#include <common/Scene.hpp>
#include <common/Shader.hpp>
#include <common/Texture.hpp>
#include <common/TextureShader.hpp>
#include <common/Triangle.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

GLFWwindow* window;

// Helper function to initialize the window
bool initWindow(std::string windowName);

// (Transforms and filtering features removed)

/* ------------------------------------------------------------------------- */
/* main                                                                      */
/* ------------------------------------------------------------------------- */
int main(int argc, char** argv) {
    // Make constants with the cameramatrix and distortion coefficients, with
    // them being the following:
    // <camera_matrix type_id="opencv-matrix">
    //   <rows>3</rows>
    //   <cols>3</cols>
    //   <dt>d</dt>
    //   <data>
    //     2218.397864043568 0. 959.5 0. 2218.397864043568 539.5 0.
    //     0. 1.</data></camera_matrix>
    // <distortion_coefficients type_id="opencv-matrix">
    //   <rows>5</rows>
    //   <cols>1</cols>
    //   <dt>d</dt>
    //   <data>
    //     -0.17611576780242291 1.7357972971751359 0. 0.
    //     -5.4837634455342661</data></distortion_coefficients>
    cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) << 2218.397864043568, 0.,
                            959.5, 0., 2218.397864043568, 539.5, 0., 0., 1.);
    cv::Mat distCoeffs = (cv::Mat_<double>(5, 1) << -0.17611576780242291,
                          1.7357972971751359, 0., 0., -5.4837634455342661);

    // Open camera
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open camera. Exiting." << endl;
        return -1;
    }
    cout << "Camera opened successfully." << endl;

    // Initialize OpenGL context
    if (!initWindow("Camera")) return -1;

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        fprintf(stderr, "Failed to initialize OpenGL context (GLAD)\n");
        cap.release();
        return -1;
    }
    cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "."
         << GLAD_VERSION_MINOR(version) << "\n";

    // Basic OpenGL setup
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glClearColor(0.1f, 0.1f, 0.2f, 0.0f);  // A dark blue background
    glEnable(GL_DEPTH_TEST);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Prepare Scene, Shaders, and Objects

    // We get one frame from the camera to determine its size.
    cv::Mat frame;
    cap >> frame;
    if (frame.empty()) {
        cerr << "Error: couldn't capture an initial frame from camera. "
                "Exiting.\n";
        cap.release();
        glfwTerminate();
        return -1;
    }

    // Create objects needed for rendering.
    TextureShader* textureShader =
        new TextureShader("videoTextureShader.vert", "videoTextureShader.frag");
    Scene* myScene = new Scene();
    Camera* renderingCamera = new Camera();
    // renderingCamera->setPosition(glm::vec3(0, 0, -2.5));

    // Replace the camera's projection matrix with one derived from the
    // calibrated OpenCV intrinsics so 3D overlays align with the video.
    // Extract intrinsics
    double fx = cameraMatrix.at<double>(0, 0);
    double fy = cameraMatrix.at<double>(1, 1);
    double cx = cameraMatrix.at<double>(0, 2);
    double cy = cameraMatrix.at<double>(1, 2);
    float width = (float)frame.cols;
    float height = (float)frame.rows;
    float near = 0.01f;
    float far = 1000.0f;

    glm::mat4 projection = glm::mat4(0.0f);
    projection[0][0] = (float)(2.0 * fx / width);
    projection[1][1] = (float)(2.0 * fy / height);
    projection[2][0] = (float)(2.0 * cx / width - 1.0);
    // Note: OpenCV's origin is top-left; NDC origin is center. The sign
    // below may be tuned if the projection appears vertically flipped.
    projection[2][1] = (float)(2.0 * cy / height - 1.0);
    projection[2][2] = -(far + near) / (far - near);
    projection[2][3] = -1.0f;
    projection[3][2] = (float)(-2.0 * far * near / (far - near));

    // Replace renderingCamera with one that uses the computed projection
    // while preserving the current view matrix.
    glm::mat4 viewMat = renderingCamera->getViewMatrix();
    delete renderingCamera;
    renderingCamera = new Camera(projection, viewMat);
    // renderingCamera->setPosition(glm::vec3(0, 0, -2.5));

    // Calculate aspect ratio (width / height) and create a quad with the
    // correct dimensions.
    float videoAspectRatio = (float)frame.cols / (float)frame.rows;
    cout << "Video aspect ratio: " << videoAspectRatio << endl;
    Quad* myQuad = new Quad(videoAspectRatio);
    myQuad->setShader(textureShader);
    myScene->addObject(myQuad);

    // Create a vertex-color shader for the cube so each face can have
    // independent vertex colors.
    Shader* vertexColorShader =
        new Shader("vertexColorShader.vert", "vertexColorShader.frag");

    // Create an example cube, place it slightly to the right and a bit
    // closer to the camera so it overlays the video.
    Cube* myCube = new Cube();
    myCube->setShader(vertexColorShader);
    // Translate to the right (x), keep centered in y, slightly closer in z
    myCube->setTranslate(glm::vec3(0.8f, 0.0f, -0.2f));
    // Make the cube smaller
    myCube->setScale(0.3f);
    myScene->addObject(myCube);

    // This variable will hold our OpenGL texture.
    Texture* videoTexture = nullptr;

    // Flip image on both axis
    cv::flip(frame, frame, -1);
    videoTexture = new Texture(frame.data, frame.cols, frame.rows, true);

    // We must tell the shader which texture to use.
    textureShader->setTexture(videoTexture);

    // Main Render Loop
    while (!glfwWindowShouldClose(window)) {
        cap >> frame;

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Check for ESC key press
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Update the texture with a new frame from the camera
        if (!frame.empty() && videoTexture != nullptr) {
            cv::flip(frame, frame, -1);
            // Find a chessboard checkerboard pattern in the frame
            std::vector<cv::Point2f> corners;
            cv::Mat grayFrame;
            cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
            bool patternFound = findChessboardCorners(
                grayFrame, cv::Size(6, 9), corners,
                cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE |
                    cv::CALIB_CB_FAST_CHECK);

            // Print whether pattern was found
            std::cout << "Chessboard pattern found: "
                      << (patternFound ? "YES" : "NO") << std::endl;

            if (patternFound) {
                cv::cornerSubPix(grayFrame, corners, cv::Size(11, 11),
                                 cv::Size(-1, -1),
                                 cv::TermCriteria(cv::TermCriteria::EPS +
                                                      cv::TermCriteria::COUNT,
                                                  30, 0.1));
                // Find the rotation and translation vectors using solvePnP
                std::vector<cv::Point3f> objectPoints;
                for (int i = 0; i < 9; i++) {
                    for (int j = 0; j < 6; j++) {
                        objectPoints.push_back(
                            cv::Point3f(j * 0.025f, i * 0.025f, 0.0f));
                    }
                }
                cv::Mat rvec, tvec;
                cv::solvePnP(objectPoints, corners, cameraMatrix, distCoeffs,
                             rvec, tvec);

                // Project 3d points to image plane
                std::vector<cv::Point3f> axisPoints;
                axisPoints.push_back(cv::Point3f(0, 0, 0));
                axisPoints.push_back(cv::Point3f(0.075f, 0, 0));
                axisPoints.push_back(cv::Point3f(0, 0.075f, 0));
                axisPoints.push_back(cv::Point3f(0, 0, -0.075f));
                std::vector<cv::Point2f> imagePoints;

                cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix,
                                  distCoeffs, imagePoints);

                // Draw the axes on the frame
                cv::line(frame, imagePoints[0], imagePoints[1],
                         cv::Scalar(0, 0, 255), 10);
                cv::line(frame, imagePoints[0], imagePoints[2],
                         cv::Scalar(0, 255, 0), 10);
                cv::line(frame, imagePoints[0], imagePoints[3],
                         cv::Scalar(255, 0, 0), 10);

                // Convert rvec/tvec (object->camera) to a model matrix in world
                // coordinates and apply to the cube so it follows the
                // checkerboard. rvec/tvec are CV types (double). Convert
                // rotation via Rodrigues.
                cv::Mat R_cv;
                cv::Rodrigues(rvec, R_cv);

                // Print transform:
                cout << "Cube transform:\n";
                glm::mat4 transform = myCube->getTransform();
                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        cout << transform[c][r] << " ";
                    }
                    cout << endl;
                }

                // Apply to cube. Use addTransform to replace the cube's
                // transform so it follows the checkerboard.
                // myCube->addTransform(modelWorld);
            }

            // Upload the frame to the GPU
            videoTexture->update(frame.data, frame.cols, frame.rows, true);
        }

        // Render the scene from the camera's point of view
        // Bind the quad's shader (no transform/filter uniforms)
        myQuad->bindShaders();
        myScene->render(renderingCamera);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup -----------------------------------------------------------
    cout << "Closing application..." << endl;
    cap.release();
    delete myScene;
    delete renderingCamera;
    delete videoTexture;

    glfwTerminate();
    return 0;
}

/* ------------------------------------------------------------------------- */
/* Helper: initWindow (GLFW)                                                 */
/* ------------------------------------------------------------------------- */
bool initWindow(std::string windowName) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1024, 768, windowName.c_str(), NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    return true;
}
