cmake_minimum_required(VERSION 3.10)
project(imgui_vendor)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Core sources
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)
# include demo file if present
if(EXISTS "${IMGUI_DIR}/imgui_demo.cpp")
  list(APPEND IMGUI_SOURCES ${IMGUI_DIR}/imgui_demo.cpp)
endif()

# Backends
set(IMGUI_BACKENDS
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_BACKENDS})

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Use Glad as OpenGL loader in your project; define loader macro for backend
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

# Link dependencies to match top-level project usage
# The top-level CMake lists links 'glfw' and ${OPENGL_LIBRARY}
if(TARGET glfw)
  target_link_libraries(imgui PUBLIC glfw ${OPENGL_LIBRARY})
else()
  target_link_libraries(imgui PUBLIC ${OPENGL_LIBRARY})
endif()

set_target_properties(imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)
