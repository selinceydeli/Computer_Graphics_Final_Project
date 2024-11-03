#ifndef PARTICLE_H
#define PARTICLE_H

#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
DISABLE_WARNINGS_POP()

struct Particle {
    glm::vec3 position;
    glm::vec3 speed;
    glm::vec4 color;
    float     life;
  
    Particle() 
      : position(0.0f), speed(0.0f), color(1.0f), life(0.0f) { }
};  

// Method for getting the next particle index to be eliminated
unsigned int nextEliminatedParticle();

// Method for updating the values of the eliminated particle
void updateParticleValues(Particle &particle, glm::vec2 offset);

#endif // PARTICLE_H


