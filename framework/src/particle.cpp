#include "particle.h"
#include <vector>
#include <glm/glm.hpp>
#include <cstdlib>  
#include <ctime>

unsigned int lastEliminatedParticle = 0; 

// Method for getting the next particle index to be eliminated from the scene
unsigned int nextEliminatedParticle(unsigned int particleNum, std::vector<Particle>& particlesArray) {
    unsigned int idx = 0;
    for (idx = lastEliminatedParticle; idx < particleNum; idx++) {
        if (particlesArray[idx].life <= 0.0f) {
            lastEliminatedParticle = idx;
            return idx;
        }
    }
    for (idx = 0; idx < lastEliminatedParticle; idx++) {
        if (particlesArray[idx].life <= 0.0f) {
            lastEliminatedParticle = idx;
            return idx;
        }
    }
    lastEliminatedParticle = 0;
    return 0;
}

// Method for initializing the values for a new particle
void setParticleValues(Particle &particle, glm::vec2 offset) {
    particle.life = 1.0f;
    float brightness = 0.5f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.5f));
    particle.color = glm::vec4(brightness, brightness * 0.4f, 0.0f, 1.0f); // Defining a reddish-yellowish color
    particle.position = glm::vec3(
        -5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f,  // x pos in range [-5, 5]
        0.0f,                                                   // keep y pos on the plane level
        -5.0f + static_cast<float>(rand()) / RAND_MAX * 10.0f   // z pos in range [-5, 5]
    );
    particle.speed = glm::vec3(0.0f);
    /*
    particle.speed = glm::vec3(
        static_cast<float>(rand()) / RAND_MAX * 0.2f - 0.1f, // Small random x velocity
        0.5f + static_cast<float>(rand()) / RAND_MAX * 0.5f,  // Upward y velocity in range [0.5, 1.0]
        static_cast<float>(rand()) / RAND_MAX * 0.2f - 0.1f   // Small random z velocity
    );
    */
}

void initializeRandomSeed() {
    std::srand(static_cast<unsigned int>(std::time(0)));
}

