#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

struct Particle {
    glm::vec2 position;
    glm::vec2 speed;
    glm::vec4 color;
    float     life;
  
    Particle() 
      : position(0.0f), speed(0.0f), color(1.0f), life(0.0f) { }
};  


