# AR Checkerboard — Augmented Reality Cube

This project detects a printed checkerboard in a live camera feed and renders a 3D cube on top of the board using OpenGL — a simple AR demo intended for running with a phone or webcam.

Files of interest:
- `AR/AR.cpp` — main AR app: captures camera frames, detects a 9×6 chessboard, solves the pose and renders a cube using the detected pose.
- `AR/shaders/` — GLSL shader files used by the AR program.
- `CameraCalibration/` — camera calibration sample and configuration files. The repo contains `out_camera_data.xml` from a previous calibration.

This README explains how to build and run the AR window on macOS (zsh). Adjust paths/commands for Linux or Windows as needed.

**Prerequisites**

- Xcode command-line tools (for compiler and make):

  ```zsh
  xcode-select --install
  ```

- Homebrew (recommended) and the following packages:

  ```zsh
  brew install cmake pkg-config glfw glm opencv
  ```

Notes:
- `glad` is included in `external/glad/` in the repo. The project uses GLFW, GLM and OpenCV. If you installed OpenCV via Homebrew, CMake should find it through `pkg-config` / CMake find modules.

**Build**

1. Create a build directory and run CMake (out-of-source build):

```zsh
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

2. After a successful build the following executables are available in `build/`:
- `AR` — the AR demo that overlays a cube on a detected checkerboard.
- `CameraCalibration` — camera calibration utility (uses `CameraCalibration/default.xml` by default).

**Run the AR window**

The `AR` program loads shaders from a relative `shaders/` directory in the current working directory. The repository stores shaders in `AR/shaders/`, so run the binary from the `AR/` folder so the shader paths resolve correctly:

```zsh
cd AR
../build/AR
```

Alternative: run the binary from `build/` after copying the shaders into `build/`:

```zsh
cp -r ../AR/shaders ./shaders
./AR
```

What the AR app does:
- Captures frames with OpenCV (`cv::VideoCapture cap(0)` by default).
- Detects a 9×6 checkerboard (the code uses `cv::findChessboardCorners` with `cv::Size(9,6)`).
- Estimates the camera pose (`cv::solvePnP`) using a board square size of 0.025 m (2.5 cm) and hardcoded camera intrinsics in `AR/AR.cpp`.
- Renders the camera frame as a textured background and draws a 3D cube aligned to the detected board pose.

**Camera selection / using a phone as camera**

- To use a different camera index (e.g. built-in vs external), edit `AR/AR.cpp` and change the `cv::VideoCapture cap(0);` call to use the desired index (0, 1, ...), then rebuild.
- To use a phone as a camera, simplest options are:
  - Install a camera-streaming app on the phone (Android apps like IP Webcam, or other MJPEG/RTSP streamers). Point OpenCV to the stream URL by replacing the capture with a URL, for example:

    ```cpp
    // use an MJPEG stream from a phone
    cv::VideoCapture cap("http://192.168.1.42:8080/video");
    ```

    Rebuild after changing the code.
  - Alternatively use a virtual webcam driver (third-party apps) that exposes the phone as a webcam device and then use the device index as above.

**Camera calibration & using calibrated intrinsics**

The AR demo currently uses a hardcoded camera matrix and distortion coefficients inside `AR/AR.cpp`. If you want correct, device-specific intrinsics you should calibrate your camera and apply those parameters.

1. Run the calibration utility (from `build/`) with the provided settings file:

```zsh
# from the repo root
cd build
./CameraCalibration ../CameraCalibration/default.xml
```

2. Follow the instructions in the calibration window. Press `g` to start capturing frames for calibration, `u` toggles showing undistorted result, and `ESC` quits. When calibration completes it writes the camera parameters to the configured output file (e.g. `out_camera_data.xml`).

3. To use the produced intrinsics in the AR app, either:
- Edit `AR/AR.cpp` and replace the hardcoded `cameraMatrix` / `distCoeffs` with values from `CameraCalibration/out_camera_data.xml`, or
- Modify `AR/AR.cpp` to read the XML/YAML at startup. Example snippet to load params:

```cpp
cv::FileStorage fs("../CameraCalibration/out_camera_data.xml", cv::FileStorage::READ);
if (fs.isOpened()) {
  fs["camera_matrix"] >> cameraMatrix;
  fs["distortion_coefficients"] >> distCoeffs;
  fs.release();
}
```

Rebuild the `AR` executable after making changes.

**Runtime controls**

- `ESC` — Quit the AR window.
- CameraCalibration program: `g` to start capture, `u` toggle undistorted preview, `ESC` quit.

**Checkerboard details**

- The AR detector in `AR/AR.cpp` expects a 9×6 chessboard (9 inner corners across, 6 inner corners down) and the square size used in pose computation is `0.025` meters (2.5 cm). If you print your own board, either scale the square size in the code or print the board at the expected physical size.

**Troubleshooting**

- Shader files not found: remember to run the binary with a working directory that contains `shaders/` (see Run section). If you see errors like "Failed to open shader file: shaders/..." make sure to run from `AR/` or copy `AR/shaders` into your working directory.
- No checkerboard detected: ensure the entire 9×6 pattern is visible, evenly lit and in focus. Try lowering the camera resolution or moving closer/farther.
- Wrong pose or cube floating: make sure the used camera intrinsics match your camera. Run `CameraCalibration` and use the produced `out_camera_data.xml`.
