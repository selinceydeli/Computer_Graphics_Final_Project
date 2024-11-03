#ifndef PARTICLE_H
#define PARTICLE_H

#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <vector>
DISABLE_WARNINGS_POP()

struct Particle {
    glm::vec2 position;
    glm::vec2 speed;
    glm::vec4 color;
    float     life;
  
    Particle() 
      : position(0.0f), speed(0.0f), color(1.0f), life(0.0f) { }
};  

unsigned int nextEliminatedParticle(unsigned int particleNum, std::vector<Particle>& particlesArray);
void setParticleValues(Particle &particle, glm::vec2 offset);
void initializeRandomSeed();

extern unsigned int lastEliminatedParticle; // Global variable

#endif // PARTICLE_H
