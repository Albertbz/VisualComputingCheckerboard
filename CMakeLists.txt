# CMake entry point
cmake_minimum_required(VERSION 3.10)

include(CMakePrintHelpers)
project(VC_IntroOpenGL)

cmake_print_variables(CMAKE_PREFIX_PATH)
cmake_print_variables(CMAKE_SOURCE_DIR)

# --- Dependencies ---
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(
    ${GLM_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    "external"
    ${GLFW_INCLUDE_DIRS}
    # include
    .
)

# Use experimental glm features
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw
    ${OpenCV_LIBS}
)

add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

add_executable(AR
    AR/AR.cpp
    external/glad/glad.c
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(AR
    ${ALL_LIBS}
)

# Ensure the AR target can find the vendored ImGui headers
target_include_directories(AR PRIVATE
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
)

add_executable(CameraCalibration
    CameraCalibration/camera_calibration.cpp
)
target_link_libraries(CameraCalibration
    ${ALL_LIBS}
)

# --------------------------------------------------------------------------
# Source grouping for IDE organization
# --------------------------------------------------------------------------
SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*")
SOURCE_GROUP(shaders REGULAR_EXPRESSION ".*/.*shader$")
