#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>


struct Particle {
    glm::vec2 Position, Velocity;
    glm::vec4 Color;
    float     Life;
  
    Particle() 
      : Position(0.0f), Velocity(0.0f), Color(1.0f), Life(0.0f) { }
};  