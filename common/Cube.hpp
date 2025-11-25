#ifndef CUBE_HPP
#define CUBE_HPP

#include "Object.hpp"
// Include GLEW
// #include <GL/glew.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/quaternion.hpp>
class Cube : public Object {
   public:
    //! Default constructor
    /*! Setting up default cube. */
    Cube();
    //! Destructor
    /*! Delete cube. */
    ~Cube();
    //! init
    /*! Setting up default cube. */
    void init();
    //! render
    /*! Render default cube. */
    void render(Camera* camera);

    void setTransform(glm::mat4 mat);

   private:
    // 36 vertices * 3 components = 108 floats (6 faces, 2 triangles per face)
    GLfloat g_vertex_buffer_data[108];
    // Per-vertex color data (r,g,b) for each of the 36 vertices
    GLfloat g_color_buffer_data[108];
    GLuint vertexbuffer;
    GLuint colorbuffer;
};

#endif
