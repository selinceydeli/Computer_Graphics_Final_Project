// #include "gpu_mesh.h"
// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// #define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <cassert>
#include <cstdlib> // EXIT_FAILURE
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
// #include <framework/particle.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <toml/toml.hpp>
#include <vector>
#include <array>
#include <filesystem> 
#include <chrono>     
#include <thread>
#include <glm/glm.hpp>
#include <cmath>


struct Node {
    float radius {1.0f};
    float spinPeriod {0.0f};
    int orbitAround {-1};
    float orbitAltitude {0.0f};
    float orbitPeriod {0.0f};
    Node* parent = nullptr;
    std::string name;

    int meshIndex;
    //Mesh mesh;
    glm::mat4 localTransform = glm::identity<glm::mat4>();
    glm::mat4 worldTransform = glm::identity<glm::mat4>();
    std::vector<Node*> children = {};

    void update(const glm::mat4& parentTransform) {
        worldTransform = parentTransform * localTransform;
        localTransform = worldTransform;
        // for(Node* child: children) {
        //     child -> update(worldTransform);
        // }
        // for (auto& vertex : mesh.vertices) {
        //     vertex.position = glm::vec3(worldTransform * glm::vec4(vertex.position, 1.0f));
        //     vertex.normal = glm::mat3(glm::transpose(glm::inverse(worldTransform))) * vertex.normal; // Correct normal
        // }
    }
};
// Constants
float rotationSpeed = 0.0005f; // Orbit speed for bigIndependentSphere
float bigSphereSpeed = -0.25f;            // Opposite orbit speed for bigSphere
float mediumSphereSpeed = 0.5f;          // Orbit speed for mediumSphere around bigSphere
float smallSphereSpeed1 = 1.0f;         // Orbit speed for smallSphere1 around mediumSphere
float smallSphereSpeed2 = -0.75f;        // Opposite orbit speed for smallSphere2

float spinSpeed = 0.30f;                  // Spin speed around each sphere's axis
float spinSpeedSpheres = 0.50f; 


// Function to compute transformation matrices for each sphere (sun & moon)
glm::mat4 getDayLightTransform(float time) {

    glm::mat4 transform = glm::mat4(1.0f);
    float orbitAngle = rotationSpeed * time;
    transform = glm::rotate(glm::mat4(1.0f), orbitAngle, glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::translate(glm::mat4(1.0f), glm::vec3(0.00250f, 0.0f, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), spinSpeed * time/10.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    return transform;

}

glm::mat4 getBigSphereTransform(float m) {
    glm::mat4 transform = glm::mat4(1.0f);
    float orbitAngle = bigSphereSpeed * m;
    transform = glm::rotate(glm::mat4(1.0f), orbitAngle, glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::translate(glm::mat4(1.0f), glm::vec3(0.00250f, 0.0f, 0.0f));// *
                //glm::rotate(glm::mat4(1.0f), SPIN_SPEED * m , glm::vec3(0.0f, 1.0f, 0.0f));
    return transform;
}

glm::mat4 getMediumSphereTransform(glm::mat4 bigSphereTransform, float m) {
    glm::mat4 transform = glm::mat4(1.0f);
    //m = 1.0f;
    float orbitAngle = mediumSphereSpeed * m;
    transform = bigSphereTransform *
                glm::rotate(glm::mat4(1.0f),orbitAngle, glm::vec3(0, 1, 0)) *
                glm::translate(glm::mat4(1.0f), glm::vec3(0.00125f, 0.0f, 0.0f));// *
                //glm::rotate(glm::mat4(1.0f), SPIN_SPEED * m, glm::vec3(0, 1, 0));
    return transform;
}

glm::mat4 getSmallSphere1Transform(glm::mat4 mediumSphereTransform, float m) {
    glm::mat4 transform = glm::mat4(1.0f);
    float orbitAngle = smallSphereSpeed1 * m;
    transform = mediumSphereTransform *
                glm::rotate(glm::mat4(1.0f), orbitAngle, glm::vec3(0, 1, 0)) *
                glm::translate(glm::mat4(1.0f), glm::vec3(0.00520f, 0.0f, 0.0f));// *
                //glm::rotate(glm::mat4(1.0f), SPIN_SPEED *m, glm::vec3(0, 1, 0));
    return transform;
}

glm::mat4 getSmallSphere2Transform(glm::mat4 mediumSphereTransform, float m) {
    glm::mat4 transform = glm::mat4(1.0f);
    float orbitAngle = smallSphereSpeed2 * m;
    transform = mediumSphereTransform *
                glm::rotate(glm::mat4(1.0f), orbitAngle, glm::vec3(0, 1, 0)) *
                glm::translate(glm::mat4(1.0f), glm::vec3(0.00120f, 0.0f, 0.0f));// *
                //glm::rotate(glm::mat4(1.0f), SPIN_SPEED * m, glm::vec3(0, 1, 0));
    return transform;
}

void updateNodeTransform(Node* node, float time) {
    //std::cout << "mesh index of parent: " << node->meshIndex << std::endl;
    //glm::rotate(matrix, time * 2 * glm::pi<float>() / body.orbitPeriod, glm::vec3(0, 1, 0))
    const glm::mat4 transform = glm::rotate(node->worldTransform, time * 2 * glm::pi<float>() / 150.0f, glm::vec3(0, 1, 0));
    node->update(transform);
    for(Node* childNode : node->children) {
        //std::cout << "mesh index of child: " << childNode->meshIndex << std::endl;
        const glm::mat4 transformChild = glm::rotate(childNode->worldTransform, time *3  * glm::pi<float>() / 50.0f, glm::vec3(0, 1, 0));
        childNode->update(transformChild);
    }
    
}


// Animated textures - global variables
float animatedTextureFrameInterval = 1.0f / 3.0f; // 3 FPS
float animatedTextureLastFrameTime = 0.0f;
int animatedTextureCurrentFrame = 0;
bool dayNight = false;


std::vector<Mesh> animationMeshes; 
std::vector<GLuint> vaos, vbos, ibos;
float frameDuration = 0.0f;  
float timeAccumulator = 0.0f;
float lastFrameTime = 0.0f;
size_t currentMeshIndex = 0;   

// Configuration
const int WIDTH = 1200;
const int HEIGHT = 800;
const unsigned int numParticles = 2000;

bool post_process = false;
bool show_imgui = true;

bool envMap = false;
bool hierarchicalTransform = false;

/*
bool debug = true;
bool diffuseLighting = false;
bool phongSpecularLighting = false;
bool blinnPhongSpecularLighting = false;
bool toonLightingDiffuse = false;
bool toonLightingSpecular = false;
bool toonxLighting = false;
*/

std::array diffuseModes {"Debug", "Lambert Diffuse", "Toon Lighting Diffuse", "Toon X Lighting", "PBR Shading", "Normal Mapping"}; std::array specularModes {"None", "Phong Specular Lighting", "Blinn-Phong Specular Lighting", "Toon Lighting Specular"}; std::array samplingModes {"Single Sample", "PCF"};bool shadows = false;bool pcf = false;bool applyTexture = false;bool multipleShadows = false;int samplingMode = 0;int diffuseMode = 0;int specularMode = 0; bool transparency = false; bool applyMinimap = false; bool applySmoothPath = false; bool showBezierCurve = false; bool moveCamera = false; bool isConstantSpeedAlongBezier = false; bool useOriginalCamera = true; bool isTopViewCamera = false; bool isFrontViewCamera = false; bool isLeftViewCamera = false; bool isRightViewCamera = false;

bool isParticleEffect = false;
bool isParticleCollision = false;

float skyboxVert[] = {
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f 
};

unsigned int skyboxIndices[] = {
    // Right 
    1, 2, 6,
    6, 5, 1,
    // Left 
    0, 4, 7,
    7, 3, 0,
    // Top
    4, 5, 6,
    6, 7, 4,
    // Bottom
    0, 3, 2,
    2, 1, 0,
    // Back
    0, 1, 5, 
    5, 4, 0,
    // Front 
    3, 7, 6, 
    6, 2, 3
};

struct {
    // Diffuse (Lambert)
    glm::vec3 kd { 0.5f };
    // Specular (Phong/Blinn Phong)
    glm::vec3 ks { 0.5f };
    float shininess = 3.0f;
    // Toon
    int toonDiscretize = 4;
    float toonSpecularThreshold = 0.49f;
    // pbr
    float roughness = 0.5;
    float metallic = 1.0;
    float intensity = 1.0;
} shadingData;
struct Texture {
    int width;
    int height;
    int channels;
    stbi_uc* texture_data;
};
// Lights
struct Light {
    glm::vec3 position;
    glm::vec3 color;
    bool is_spotlight;
    glm::vec3 direction;
    bool has_texture;
    Texture texture;
    std::string name;
};

struct Particle {
    glm::vec2 position;
    glm::vec2 speed;
    glm::vec4 color;
    float     life;
  
    Particle() 
      : position(0.0f), speed(glm::vec2(2.0, 0.0)), color(glm::vec4(1.0, 0.0, 0.0, 1.0)), life(1.0f) { }
};  

struct Obstacle {
    glm::vec2 position; 
    float radius;     

    Obstacle(glm::vec2 pos, float r)
        : position(pos), radius(r) {}
};

std::vector<Light> lights {};
size_t selectedLightIndex = 0;

std::vector<Light> secondaryLights {};
size_t selectedSecondaryLightIndex = 0;

glm::vec3 posForSun = glm::vec3(0.0f);
glm::vec3 posForMoon = glm::vec3(0.0f);
Light sunLight {posForSun, glm::vec3(1.0f,0.842f,0.24f), false, glm::vec3(0, 0, 0) };
Light moonLight {posForMoon, glm::vec3(0.15), false, glm::vec3(0, 0, 0) };

#pragma region  Particle Definition
// Particle effect helper methods
unsigned int lastEliminatedParticle;
// Method for initializing the values for a new particle
void setParticleValues(Particle &particle, glm::vec2 offset) {
    particle.life = 3.0f; // Define the lifetime of the particles
    float brightness = 0.5f - static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.5f));
    particle.color = glm::vec4(1.0, brightness * 0.2f, 0.0f, 1.0f); 
    particle.position = glm::vec2(
        offset.x + static_cast<float>(rand()) / RAND_MAX * 1.0f, 
        offset.y + static_cast<float>(rand()) / RAND_MAX * 1.0f  
    );  
    // Generate a random direction with uniform distribution
    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * glm::pi<float>();
    float speedMagnitude = static_cast<float>(rand()) / RAND_MAX * 2.0f; 
    particle.speed = glm::vec2(
        cos(angle) * speedMagnitude,
        sin(angle) * speedMagnitude
    );
}

void initializeRandomSeed() {
    std::srand(static_cast<unsigned int>(std::time(0)));
}

// Method for getting the next particle index to be eliminated from the scene
unsigned int nextEliminatedParticle(unsigned int particleNum, std::vector<Particle>& particlesArray) {
    for (unsigned int i = 0; i < particleNum; i++) {
        unsigned int idx = (lastEliminatedParticle + i) % particleNum;
        if (particlesArray[idx].life <= 0.0f) {
            lastEliminatedParticle = idx;
            return idx;
        }
    }
    lastEliminatedParticle = 0;
    return 0;
}

// Check if particle is colliding with the current obstable
bool isColliding(const glm::vec2& particlePos, const Obstacle& obstacle) {
    float dist = glm::distance(particlePos, obstacle.position);
    // std::cout<<"Current pos & dist: [" << particlePos.x<< "," << particlePos.y <<"]  " << dist  << std::endl;
    return dist <= obstacle.radius;
}

void handleCollision(Particle &particle, const Obstacle& obstacle) {
    glm::vec2 particlePos = particle.position;
    glm::vec2 particleVelocity = particle.speed;
    glm::vec2 normal = glm::normalize(particlePos - obstacle.position);
    particle.position = obstacle.position + normal * (obstacle.radius + 0.01f);
    particleVelocity = particleVelocity - 2.0f * glm::dot(particleVelocity, normal) * normal;
    float elasticity = 0.8f;
    particle.speed *= elasticity;
    // std::cout << "Handle Collision!!" << std::endl;
}

#pragma endregion

void resetLights()
{
    lights.clear();
    lights.push_back(Light { glm::vec3(0, 0, 3), glm::vec3(1) });
    selectedLightIndex = 0;
}
bool isDay = false;

//#pragma region GUI
void imgui()
{
    // Define UI here
    if (!show_imgui)
        return;

    ImGui::Begin("Final Project Part 1 : Modern Shading");
    ImGui::Text("Press \\ to show/hide this menu");

    ImGui::Separator();
    ImGui::Text("Material parameters");
    ImGui::SliderFloat("Shininess", &shadingData.shininess, 0.0f, 100.f);

    // Color pickers for Kd and Ks
    ImGui::ColorEdit3("Kd", &shadingData.kd[0]);
    ImGui::ColorEdit3("Ks/Albedo", &shadingData.ks[0]);

    ImGui::SliderInt("Toon Discretization", &shadingData.toonDiscretize, 1, 10);
    ImGui::SliderFloat("Toon Specular Threshold", &shadingData.toonSpecularThreshold, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &shadingData.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Metallic", &shadingData.metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Light Intensity", &shadingData.intensity, 1.0f, 10.0f);

    ImGui::Separator();
    ImGui::Combo("Diffuse Mode", &diffuseMode, diffuseModes.data(), (int)diffuseModes.size());

    ImGui::Separator();
    ImGui::Combo("Specular Mode", &specularMode, specularModes.data(), (int)specularModes.size());

    ImGui::Separator();
    ImGui::Checkbox("Enable Hierarchical Transformations", &hierarchicalTransform);
    if(hierarchicalTransform) {
        ImGui::Text("Rotation speed controller");
        ImGui::SliderFloat("Big sphere speed", &bigSphereSpeed, -2.5f, 2.5f);
        ImGui::SliderFloat("Medium sphere speed", &mediumSphereSpeed, -2.5f, 2.5f);
        ImGui::SliderFloat("First small sphere speed", &smallSphereSpeed1, -2.5f, 2.5f);
        ImGui::SliderFloat("Second small sphere speed", &smallSphereSpeed2, -2.5f, 2.5f);
        //ImGui::SliderFloat("Spinning speed", &spinSpeedSpheres, 0.01f, 5.0f);
    }

    ImGui::Separator();
    ImGui::Checkbox("Enable Day/Night System", &dayNight);
    if(dayNight) {
        if(isDay) ImGui::Text("Currently it is day!");
        else ImGui::Text("Currently it is night!");
        ImGui::SliderFloat("Time speed", &rotationSpeed, 0.01f, 5.0f);
    }
    
    ImGui::Separator();
    ImGui::Checkbox("Enable Particle Effect", &isParticleEffect);
    ImGui::Checkbox("Enable Particle Collision", &isParticleCollision);

    ImGui::Separator();
    ImGui::Checkbox("Enable Animated Textures to Light", &lights[selectedLightIndex].has_texture);
    ImGui::Checkbox("Enable Shadows", &shadows);
    ImGui::Checkbox("Enable PCF", &pcf);
    ImGui::Checkbox("Shadows for Multiple Light Sources", &multipleShadows);

    ImGui::Separator();
    ImGui::Text("Secondary Lights");

    std::vector<std::string> secondaryItemStrings = {};
    for (size_t i = 0; i < secondaryLights.size(); i++) {
        auto string = "Secondary Light " + std::to_string(i);
        secondaryItemStrings.push_back(string);
    }

    std::vector<const char*> secondaryItemCStrings = {};
    for (const auto& string : secondaryItemStrings) {
        secondaryItemCStrings.push_back(string.c_str());
    }

    int tempSecondarySelectedItem = static_cast<int>(selectedSecondaryLightIndex);
    if (ImGui::ListBox("Secondary Lights", &tempSecondarySelectedItem, secondaryItemCStrings.data(), (int)secondaryItemCStrings.size(), 1)) {
        selectedSecondaryLightIndex = static_cast<size_t>(tempSecondarySelectedItem);
    }

    if (!secondaryLights.empty()) {
        Light& selectedLight = secondaryLights[selectedSecondaryLightIndex];
        ImGui::SliderFloat3("Secondary Light Position", &selectedLight.position[0], -10.0f, 10.0f);
    }
    
    ImGui::Separator();
    ImGui::Checkbox("Enable Mini Map", &applyMinimap);

    ImGui::Separator();
    ImGui::Checkbox("Enable Environment Map", &envMap);

    ImGui::Separator();
    ImGui::Checkbox("Enable Post Processing", &post_process);
    
    ImGui::Separator();
    ImGui::Text("Light Movement Along the Bezier Curve");
    ImGui::Checkbox("Enable Smooth Path for Light", &applySmoothPath);
    ImGui::Checkbox("Enable Constant Speed", &isConstantSpeedAlongBezier);

    ImGui::Separator();
    ImGui::Text("Camera Movement Along the Bezier Curve");
    ImGui::Checkbox("Enable Smooth Path for Camera", &moveCamera);

    //ImGui::Checkbox("Show Bezier Path", &showBezierCurve);

    const char* cameraViews[] = { "Original", "Top View", "Front View", "Left View", "Right View" };
    static int selectedCameraIndex = 0;

    ImGui::Separator();
    ImGui::Text("Select Camera View");
    if (ImGui::Combo("Camera View", &selectedCameraIndex, cameraViews, IM_ARRAYSIZE(cameraViews))) {
        // Reset
        useOriginalCamera = false;
        isTopViewCamera = false;
        isFrontViewCamera = false;
        isLeftViewCamera = false;
        isRightViewCamera = false;

        switch (selectedCameraIndex) {
            case 0:  // Original
                useOriginalCamera = true;
                break;
            case 1:  // Top View
                isTopViewCamera = true;
                break;
            case 2:  // Front View
                isFrontViewCamera = true;
                break;
            case 3:  // Left View
                isLeftViewCamera = true;
                break;
            case 4:  // Right View
                isRightViewCamera = true;
                break;
        }
    }

    ImGui::Separator();
    ImGui::Text("Primary Lights");

    // Display lights in scene
    std::vector<std::string> itemStrings = {};
    for (size_t i = 0; i < lights.size(); i++) {
        auto string = "Light " + std::to_string(i);
        itemStrings.push_back(string);
    }

    std::vector<const char*> itemCStrings = {};
    for (const auto& string : itemStrings) {
        itemCStrings.push_back(string.c_str());
    }

    int tempSelectedItem = static_cast<int>(selectedLightIndex);
    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int) itemCStrings.size(), 4)) {
        selectedLightIndex = static_cast<size_t>(tempSelectedItem);
    }

    // Modify selected light's position and color
    if (!lights.empty()) {
        Light& selectedLight = lights[selectedLightIndex];
        ImGui::ColorEdit3("Light Color", &selectedLight.color[0]);
        ImGui::SliderFloat3("Light Position", &selectedLight.position[0], -10.0f, 10.0f);

        // Toggle whether the light is a spotlight
        ImGui::Checkbox("Is Spotlight", &selectedLight.is_spotlight);
        if (selectedLight.is_spotlight) {
            ImGui::SliderFloat3("Spotlight Direction", &selectedLight.direction[0], -1.0f, 1.0f);
        }

        // Optionally, allow editing the texture binding if the light has a texture
        if (selectedLight.has_texture) {
            ImGui::Text("Light has a texture.");
        }
    }

    // Add a new light
    if (ImGui::Button("Add Light")) {
        lights.push_back(Light { glm::vec3(0, 0, 3), glm::vec3(1) });
        selectedLightIndex = lights.size() - 1; // Select the newly added light
    }

    // Remove the selected light
    if (ImGui::Button("Remove Selected Light") && !lights.empty()) {
        lights.erase(lights.begin() + selectedLightIndex);
        if (selectedLightIndex >= lights.size()) {
            selectedLightIndex = lights.size() - 1; // Adjust selection if the last light was removed
        }
    }

    // Clear lights
    if (ImGui::Button("Reset Lights")) {
        resetLights();
    }

    ImGui::End();
    ImGui::Render();
}
//#pragma endregion

std::optional<glm::vec3> tomlArrayToVec3(const toml::array* array)
{
    glm::vec3 output {};

    if (array) {
        int i = 0;
        array->for_each([&](auto&& elem) {
            if (elem.is_number()) {
                if (i > 2)
                    return;
                output[i] = static_cast<float>(elem.as_floating_point()->get());
                i += 1;
            } else {
                std::cerr << "Error: Expected a number in array, got " << elem.type() << std::endl;
                return;
            }
        });
    }

    return output;
}

std::vector<Mesh> loadMeshesFromDirectory(const std::string& dir_path) {
    std::vector<Mesh> meshes;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.path().extension() == ".obj") {
            meshes.push_back(mergeMeshes(loadMesh(entry.path().string())));
        }
    }
    return meshes;
}

// Methods for implementing Smooth Paths with Bezier Curve 
float t = 0.0;                          // Time along the Bezier curve for light
float t_camera = 0.0;                   // Time along the Bezier curve for camera
size_t currentCurve = 0;                // Current Bezier curve
std::vector<glm::vec3> bezierCurvePath;
std::vector<glm::vec3> bezierCurvePathEven;

glm::vec3 computeBezierPoint(float t, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) { 
    float factor = (1.0 - t);
    float factorDouble = factor * factor;
    float factorTriple = factor * factor * factor;

    glm::vec3 bezier = factorTriple * p0;
    bezier += 3 * factorDouble * t * p1;
    bezier += 3 * factor * t * t * p2;
    bezier += t * t * t * p3;

    return bezier;
}

// Implement 5 consecutive bezier curves for the light motion
std::vector<std::vector<glm::vec3>> bezierControlPointsSets = {
    {
        glm::vec3(4.0f, 1.0f, 0.0f),  // p0
        glm::vec3(3.0f, 2.5f, 4.0f),  // p1
        glm::vec3(1.0f, 1.5f, 4.5f),  // p2
        glm::vec3(0.0f, 1.0f, 4.0f)   // p3
    },
    {
        glm::vec3(0.0f, 1.0f, 4.0f),
        glm::vec3(-3.0f, 1.5f, 3.0f),
        glm::vec3(-4.5f, 2.0f, 0.0f),
        glm::vec3(-4.5f, 1.0f, 0.0f)
    },
    {
        glm::vec3(-4.5f, 1.0f, 0.0f),
        glm::vec3(-3.0f, 2.0f, -3.0f),
        glm::vec3(0.0f, 1.5f, -4.0f),
        glm::vec3(4.0f, 1.0f, 0.0f)
    },
    {
        glm::vec3(4.0f, 1.0f, 0.0f),  
        glm::vec3(2.0f, 2.0f, -4.0f),  
        glm::vec3(-2.0f, 1.5f, -4.0f), 
        glm::vec3(-4.0f, 1.0f, 0.0f)   
    },
    {
        glm::vec3(-4.0f, 1.0f, 0.0f),   
        glm::vec3(-2.0f, 1.5f, 2.0f),   
        glm::vec3(2.0f, 1.5f, 2.0f),    
        glm::vec3(4.0f, 1.0f, 0.0f)    
    }
};
 
// Bezier control points for the middle plane movement of the camera
std::vector<glm::vec3> middlePlaneBezierCurve = 
{
    glm::vec3(0.0f, 0.0f, 5.0f),    
    glm::vec3(0.0f, 1.0f, 2.0f),    
    glm::vec3(0.0f, 1.0f, 0.0f),   
    glm::vec3(0.0f, 0.0f, -1.0f)    
};

void changeLightPosAlongBezierCurves(float timeChange) {
    if (currentCurve < bezierControlPointsSets.size()) {
        float speedFactor = 0.1;
        t += timeChange * speedFactor;
        if (t > 1.0) {
            t = 0.0;
            currentCurve += 1;

            if (currentCurve >= bezierControlPointsSets.size()) {
                currentCurve = 0;  
            }
        }
        const auto& controlP = bezierControlPointsSets[currentCurve];
        lights[selectedLightIndex].position = computeBezierPoint(t, controlP[0], controlP[1], controlP[2], controlP[3]);
    } 
    return;
}

// Methods for calculating constant speed along a Bezier Curve
std::vector<glm::vec3> computeEvenSpacedPoints(const std::vector<glm::vec3>& controlPoints, float spacing, float resolution = 1.0f) {
    std::vector<glm::vec3> pointsWithEvenSpace;
    pointsWithEvenSpace.push_back(controlPoints[0]);
    glm::vec3 prevPoint = controlPoints[0];
    float dist = 0.0;

    float totalDistAlongCurve = glm::distance(controlPoints[0], controlPoints[1]) + 
                                glm::distance(controlPoints[1], controlPoints[2]) + 
                                glm::distance(controlPoints[2], controlPoints[3]);
    float estimatedDist = glm::distance(controlPoints[0], controlPoints[3]) + totalDistAlongCurve / 2.0f;

    float time = 0.0;
    while (time <= 1.0) {
        time += 1.0 / std::ceil(estimatedDist * resolution * 10);        
        glm::vec3 currentPoint = computeBezierPoint(time, controlPoints[0], controlPoints[1], controlPoints[2], controlPoints[3]);
        dist += glm::distance(prevPoint, currentPoint);
        
        while (dist >= spacing) {
            float overshootDistance = dist - spacing;
            glm::vec3 newPoint = currentPoint + (prevPoint - currentPoint) * (overshootDistance / glm::distance(prevPoint, currentPoint));
            pointsWithEvenSpace.push_back(newPoint);
            dist = overshootDistance;
            prevPoint = newPoint;
        }

        prevPoint = currentPoint;
    }
    return pointsWithEvenSpace;
}

void initializeEvenSpacedCurved(float spacing, float resolution) {
    bezierCurvePathEven.clear();  
    for (const auto& controlPoints : bezierControlPointsSets) {
        std::vector<glm::vec3> segmentPoints = computeEvenSpacedPoints(controlPoints, spacing, resolution);
        bezierCurvePathEven.insert(bezierCurvePathEven.end(), segmentPoints.begin(), segmentPoints.end());
    }
}

// Interpolation for a smoother path
glm::vec3 interpolate(const glm::vec3& start, const glm::vec3& end, float t) {
    return (1.0f - t) * start + t * end;
}


void moveLightAlongEvenlySpacedPath(float timeChange) {
    static size_t idx = 0;
    static float accumulatedDist = 0.0f;
    float speed = 0.5f;  

    if (bezierCurvePathEven.empty()) return;
    accumulatedDist += timeChange * speed;

    while (accumulatedDist > 0.0f && idx < bezierCurvePathEven.size() - 1) {
        glm::vec3 firstPoint = bezierCurvePathEven[idx];
        glm::vec3 secondPoint = bezierCurvePathEven[idx+1];
        if (accumulatedDist >= glm::distance(firstPoint, secondPoint)) {
            accumulatedDist -= glm::distance(firstPoint, secondPoint);
            idx++;
        } else {
            float time = accumulatedDist / glm::distance(firstPoint, secondPoint);
            lights[selectedLightIndex].position = interpolate(firstPoint, secondPoint, time);
            return; 
        }
    }

    if (idx >= bezierCurvePathEven.size() - 1) idx = 0;
    lights[selectedLightIndex].position = bezierCurvePathEven[idx];
}

// Program entry point. Everything starts here.
int main(int argc, char** argv)
{

    // read toml file from argument line (otherwise use default file)
    std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/final.toml";
    //std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/default_scene.toml"; // Scene for animation
    // std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/pbr_test.toml";
    //std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/normal_mapping.toml";

    // parse initial scene config
    toml::table config;
    try {
        config = toml::parse_file(std::string(RESOURCE_ROOT) + config_filename);
    } catch (const toml::parse_error& ) {
        std::cerr << "parsing failed" << std::endl;
    }

    // read material data
    shadingData.kd = tomlArrayToVec3(config["material"]["kd"].as_array()).value();
    shadingData.ks = tomlArrayToVec3(config["material"]["ks"].as_array()).value();
    shadingData.shininess = config["material"]["shininess"].value_or(0.0f);
    shadingData.roughness = config["material"]["roughness"].value_or(0.0f);
    shadingData.metallic = config["material"]["metallic"].value_or(0.0f);
    shadingData.intensity = config["material"]["intensity"].value_or(1.0f);
    shadingData.toonDiscretize = (int) config["material"]["toonDiscretize"].value_or(0);
    shadingData.toonSpecularThreshold = config["material"]["toonSpecularThreshold"].value_or(0.0f);

    // read lights (primary)
    lights = std::vector<Light> {};
    size_t num_lights = config["lights"]["positions"].as_array()->size();
    std::cout << num_lights << std::endl;

    for (size_t i = 0; i < num_lights; ++i) {
        auto pos = tomlArrayToVec3(config["lights"]["positions"][i].as_array()).value();
        auto color = tomlArrayToVec3(config["lights"]["colors"][i].as_array()).value();
        bool is_spotlight = config["lights"]["is_spotlight"][i].value<bool>().value();
        auto direction = tomlArrayToVec3(config["lights"]["direction"][i].as_array()).value();
        bool has_texture = config["lights"]["has_texture"][i].value<bool>().value();

        auto tex_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
        int width = 0, height = 0, sourceNumChannels = 0;// Number of channels in source image. pixels will always be the requested number of channels (3).
        stbi_uc* pixels = nullptr;        
        if (has_texture) {
            pixels = stbi_load(tex_path.c_str(), &width, &height, &sourceNumChannels, STBI_rgb);
        }
        applyTexture = has_texture;

        lights.emplace_back(Light { pos, color, is_spotlight, direction, has_texture, { width, height, sourceNumChannels, pixels } });
    }

    // Add only one secondary light source
    secondaryLights = std::vector<Light> {};
    glm::vec3 pos = glm::vec3(-3.766f, 3.243f, -0.503f);
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction = glm::normalize(target - pos);
    auto color = tomlArrayToVec3(config["lights"]["colors"][0].as_array()).value();
    bool is_spotlight = config["lights"]["is_spotlight"][0].value<bool>().value();
    bool has_texture = config["lights"]["has_texture"][0].value<bool>().value();

    auto tex_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
    int width = 0, height = 0, sourceNumChannels = 0;// Number of channels in source image. pixels will always be the requested number of channels (3).
    stbi_uc* pixels = nullptr;        
    if (has_texture) {
        pixels = stbi_load(tex_path.c_str(), &width, &height, &sourceNumChannels, STBI_rgb);
    }
    applyTexture = has_texture;
    secondaryLights.emplace_back(Light { pos, color, is_spotlight, direction, has_texture, { width, height, sourceNumChannels, pixels } });

    // Create window
    Window window { "Shading", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };

    // read camera settings
    auto look_at = tomlArrayToVec3(config["camera"]["lookAt"].as_array()).value();
    auto rotations = tomlArrayToVec3(config["camera"]["rotations"].as_array()).value();
    float fovY = config["camera"]["fovy"].value_or(50.0f);
    float dist = config["camera"]["dist"].value_or(1.0f);

    auto diffuse_model = config["render_settings"]["diffuse_model"].value<std::string>();
    auto specular_model = config["render_settings"]["specular_model"].value<std::string>();
    bool do_pcf = config["render_settings"]["pcf"].value<bool>().value();
    bool do_shadows = config["render_settings"]["shadows"].value<bool>().value();

    shadows = do_shadows;
    pcf = do_pcf;

    if (diffuse_model == "debug") {
        diffuseMode = 0;
    } else if (diffuse_model == "lambert") {
        diffuseMode = 1;
    } else if (diffuse_model == "toon") {
        diffuseMode = 2;
    } else if (diffuse_model == "x-toon") {
        diffuseMode = 3;
    } else if (diffuse_model == "pbr") {
        diffuseMode = 4;
    } else if (diffuse_model == "normal-mapping"){
        diffuseMode = 5;
    } else{
        diffuseMode = 0;
    }

    if (specular_model == "none") {
        specularMode = 0;
    } else if (specular_model == "phong") {
        specularMode = 1;
    } else if (specular_model == "blinn-phong") {
        specularMode = 2;
    } else {
        specularMode = 3;
    }

    // Define multiple views
    Trackball mainCamera { &window, glm::radians(fovY) };
    mainCamera.setCamera(look_at, rotations, dist);

    Trackball topViewCamera { &window, glm::radians(fovY) };
    glm::vec3 topViewRotations = glm::vec3(glm::radians(90.0f), 0.0f, 0.0f); 
    topViewCamera.setCamera(look_at, topViewRotations, dist);

    Trackball rightViewCamera { &window, glm::radians(fovY) };
    glm::vec3 rightViewRotations = glm::vec3(0.0f, glm::radians(90.0f), 0.0f);
    rightViewCamera.setCamera(look_at, rightViewRotations, dist);

    Trackball leftViewCamera { &window, glm::radians(fovY) };
    glm::vec3 leftViewRotations = glm::vec3(0.0f, glm::radians(-90.0f), 0.0f);
    leftViewCamera.setCamera(look_at, leftViewRotations, dist);

    Trackball frontViewCamera { &window, glm::radians(fovY) };
    glm::vec3 frontViewRotations = glm::vec3(0.0f, glm::radians(0.0f), 0.0f);
    frontViewCamera.setCamera(look_at, frontViewRotations, dist);

    // Define pointers to switch between active cameras
    Trackball* activeCamera = &mainCamera;
    Trackball* topViewCameraPtr = &topViewCamera;
    Trackball* rightViewCameraPtr = &rightViewCamera;
    Trackball* leftViewCameraPtr = &leftViewCamera;
    Trackball* frontViewCameraPtr = &frontViewCamera;

    // read mesh
    bool animated = config["mesh"]["animated"].value_or(false);
    auto mesh_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
    std::cout << mesh_path << std::endl;
    

    // Initialize the Bezier Curve for constant speed movement
    float spacing = 0.1f;      
    float resolution = 1.0f;   
    initializeEvenSpacedCurved(spacing, resolution);


    // Initialize the particle array
    initializeRandomSeed();
    const float dt = 0.01;
    std::vector<Particle> particles;
  
    glm::vec2 particleEmitter = glm::vec2(0.0f, 0.0f);
    for (unsigned int i = 0; i < numParticles; ++i)
    {
        particles.push_back(Particle());
        setParticleValues(particles[i], particleEmitter);
    }

    std::vector<Obstacle> obstacles;
    obstacles.push_back(Obstacle(glm::vec2(1.8f, 1.8f), 0.5f));
    obstacles.push_back(Obstacle(glm::vec2(-1.0f, -1.0f), 0.8f));
    obstacles.push_back(Obstacle(glm::vec2(1.2f, -1.2f), 0.5f));
    obstacles.push_back(Obstacle(glm::vec2(-1.0f, 1.5f), 0.6f));

    //#pragma region Render
    // Shading functionality
        const Shader debugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl").build();
        const Shader lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl").build();
        const Shader lambertShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lambert_frag.glsl").build();
        const Shader phongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/phong_frag.glsl").build();
        const Shader blinnPhongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_phong_frag.glsl").build();
        const Shader toonDiffuseShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_diffuse_frag.glsl").build();
        const Shader toonSpecularShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_specular_frag.glsl").build();
        const Shader xToonShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/xtoon_frag.glsl").build();
        const Shader pbrShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/pbr_frag.glsl").build();   

        const Shader normalShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/normal_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/normal_frag.glsl").build();
        const Shader minimapShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/minimap_frag.glsl").build(); 
        const Shader mapShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/map_frag.glsl").build(); 
        
        const Shader mainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").build();
        const Shader shadowShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").build();
        const Shader lightTextureShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_texture_frag.glsl").build();
        const Shader secondaryMainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader2_frag.glsl").build();
        const Shader shadowShader2 = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow2_frag.glsl").build();

        const Shader screenShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/post_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/post_frag.glsl").build();
        const Shader particleShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/particle_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/particle_frag.glsl").build();
              
        const Shader envMapShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl").build();  
        const Shader otherEnvMapShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/env_map_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/env_map_frag.glsl").build();      
        

    if (!animated) {
        //const Mesh mesh = loadMesh(mesh_path)[0];
        //const Mesh mesh = mergeMeshes(loadMesh(mesh_path));
        //const Mesh mesh = mergeMeshes(loadMesh(RESOURCE_ROOT "build/resources/scene.obj"));
        //hierarchicalTransform
        std::vector<Mesh> meshes = loadMesh(mesh_path);

        Mesh mesh = mergeMeshes(meshes);
        calculateTangentsAndBitangents(mesh);

        window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
            if (key == '\\' && action == GLFW_PRESS) {
                show_imgui = !show_imgui;
            }

            if (action != GLFW_RELEASE)
                return;

            const bool shiftPressed = window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);

            switch (key) {
            case GLFW_KEY_L: {
                if (shiftPressed)
                    lights.push_back(Light { activeCamera->position(), glm::vec3(1) });
                else
                    lights[selectedLightIndex].position = activeCamera->position();
                    lights[selectedLightIndex].direction = activeCamera->forward();
                return;
            }
            case GLFW_KEY_N: {
                resetLights();
                return;
            }
            case GLFW_KEY_T: {
                if (shiftPressed)
                    shadingData.toonSpecularThreshold += 0.001f;
                else
                    shadingData.toonSpecularThreshold -= 0.001f;
                std::cout << "ToonSpecular: " << shadingData.toonSpecularThreshold << std::endl;
                return;
            }
            case GLFW_KEY_D: {
                if (shiftPressed) {
                    ++shadingData.toonDiscretize;
                } else {
                    if (--shadingData.toonDiscretize < 1)
                        shadingData.toonDiscretize = 1;
                }
                std::cout << "Toon Discretization levels: " << shadingData.toonDiscretize << std::endl;
                return;
            }
            default:
                return;
            };
        });

        
        #pragma region DefineBuffer

        // Set Quad for post processing
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f, 1.0f,  1.0f, 1.0f
        };

        GLuint quadVAO, quadVBO;
        glGenBuffers(1, &quadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &quadVAO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        
        // Set up the vertex attributes
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        glBindVertexArray(0);


        GLuint skyboxVAO, skyboxVBO, skyboxIBO;
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glGenBuffers(1, &skyboxIBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVert), &skyboxVert, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        
        std::string facesCubemap[6] = {
            std::string(RESOURCE_ROOT) + "resources/right.jpg",
            std::string(RESOURCE_ROOT) + "resources/left.jpg",
            std::string(RESOURCE_ROOT) + "resources/top.jpg",
            std::string(RESOURCE_ROOT) + "resources/bottom.jpg",
            std::string(RESOURCE_ROOT) + "resources/front.jpg",
            std::string(RESOURCE_ROOT) + "resources/back.jpg"
        };

        GLuint cubemapTexture;
        glGenTextures(1, &cubemapTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        
        for(unsigned int i = 0; i < 6; i++) {
            int width, height, nrChannels;
            stbi_uc* data = stbi_load(facesCubemap[i].c_str(), &width, &height, &nrChannels, 0);
            if(data) {
                stbi_set_flip_vertically_on_load(false);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            } else {
                std::cout << "Failed to load texture: " << facesCubemap[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);



        // Create Vertex Buffer Object and Index Buffer Objects.
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        GLuint ibo;
        // Create index buffer object (IBO)
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Bind vertex data to shader inputs using their index (location).
        // These bindings are stored in the Vertex Array Object.
        GLuint vao;
        // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

        // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
        // The stride is the distance in bytes between vertices. We use the offset to point to the normals
        // instead of the positions.
        // Tell OpenGL that we will be using vertex attributes 0 and 1.
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
        #pragma endregion

        #pragma region DefineTexture
        // Load image from disk to CPU memory.
        int width, height, sourceNumChannels; // Number of channels in source image. pixels will always be the requested number of channels (3).
        stbi_uc* pixels = stbi_load(RESOURCE_ROOT "resources/toon_map.png", &width, &height, &sourceNumChannels, STBI_rgb);

        // Create a texture on the GPU with 3 channels with 8 bits each.
        GLuint texToon;
        glGenTextures(1, &texToon);
        glBindTexture(GL_TEXTURE_2D, texToon);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        // Set behavior for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // Red brick texture
        int redBrickWidth, redBrickHeight, redBrickChannels;
        stbi_uc* red_brick_pixels = stbi_load(RESOURCE_ROOT "resources/red-textured-wall.jpg", &redBrickWidth, &redBrickHeight, &redBrickChannels, STBI_rgb);

        // Create a texture on the GPU with 3 channels with 8 bits each.
        GLuint texRedBrick;
        glGenTextures(1, &texRedBrick);
        glBindTexture(GL_TEXTURE_2D, texRedBrick);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, red_brick_pixels);

        // Set behavior for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(red_brick_pixels);


        // Rainbow texture
        int rainbowWidth, rainbowHeight, rainbowChannels;
        stbi_uc* rainbow_pixels = stbi_load(RESOURCE_ROOT "resources/textured-rainbow.jpg", &rainbowWidth, &rainbowHeight, &rainbowChannels, STBI_rgb);

        // Create a texture on the GPU with 3 channels with 8 bits each.
        GLuint texRainbow;
        glGenTextures(1, &texRainbow);
        glBindTexture(GL_TEXTURE_2D, texRainbow);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rainbowWidth, rainbowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, rainbow_pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(rainbow_pixels);


        // Create Light Texture
        int texWidth, texHeight, texChannels;
        auto light_texture_path = std::string(RESOURCE_ROOT) + config["lights"]["texture_path"][0].value_or("resources/smiley.png");
        stbi_uc* light_pixels = stbi_load(light_texture_path.c_str(), &texWidth, &texHeight, &texChannels, 3); 
        //stbi_uc* light_pixels = stbi_load(RESOURCE_ROOT "resources/smiley.png", &texWidth, &texHeight, &texChannels, 3);

        GLuint texLight;
        glGenTextures(1, &texLight);
        glBindTexture(GL_TEXTURE_2D, texLight);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, light_pixels);

        // Set behaviour for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // Create Normal Texture
        // Normal Texture
        int normalWidth, normalHeight, normalChannels;
        stbi_uc* normal_pixels = stbi_load(RESOURCE_ROOT "resources/normal_map1.png", &normalWidth, &normalHeight, &normalChannels, STBI_rgb);

        GLuint texNormal;
        glGenTextures(1, &texNormal);
        glBindTexture(GL_TEXTURE_2D, texNormal);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, normalWidth, normalHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, normal_pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(normal_pixels);

        // Material Texture
        int matWidth, matHeight, matChannels;
        stbi_uc* mat_pixels = stbi_load(RESOURCE_ROOT "resources/brickwall.jpg", &matWidth, &matHeight, &matChannels, STBI_rgb);

        GLuint texMat;
        glGenTextures(1, &texMat);
        glBindTexture(GL_TEXTURE_2D, texMat);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, matWidth, matHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, mat_pixels);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(mat_pixels);


        // === Create Shadow Texture ===
        GLuint texShadow;
        const int SHADOWTEX_WIDTH = 1024;
        const int SHADOWTEX_HEIGHT = 1024;
        glGenTextures(1, &texShadow);
        glBindTexture(GL_TEXTURE_2D, texShadow);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        // Set behaviour for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, 0);
        
        // === Create framebuffer for extra texture ===
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texShadow, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // === Create Shadow Texture for the Second Light ===
        GLuint shadowTex2;
        const int SHADOWTEX2_WIDTH = 1024;
        const int SHADOWTEX2_HEIGHT = 1024;
        glGenTextures(1, &shadowTex2);
        glBindTexture(GL_TEXTURE_2D, shadowTex2);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX2_WIDTH, SHADOWTEX2_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        // Set behaviour for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, 0);
                
        // === Create framebuffer for shadow texture 2 ===
        GLuint shadowFramebuffer2;
        glGenFramebuffers(1, &shadowFramebuffer2);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer2);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex2, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // Free the CPU memory after we copied the image to the GPU.
        stbi_image_free(pixels);
        stbi_image_free(light_pixels);


        // Create color texture for post processing
        GLuint texScreen;
        glGenTextures(1, &texScreen);
        glBindTexture(GL_TEXTURE_2D, texScreen);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create postbuffer for post processing
        GLuint postBuffer;
        glGenFramebuffers(1, &postBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, postBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texScreen, 0);

        // Depth render buffer
        GLuint depthRBO;
        glGenRenderbuffers(1, &depthRBO);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, WIDTH, HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        

        // Create a framebuffer for minimap
        GLuint minimapFBO;
        glGenFramebuffers(1, &minimapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, minimapFBO);

        GLuint minimapTexture;
        glGenTextures(1, &minimapTexture);
        glBindTexture(GL_TEXTURE_2D, minimapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, minimapTexture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // Animated textures
        std::vector<GLuint> texFrames;
        texFrames.push_back(texLight);
        //texFrames.push_back(texToon);
        texFrames.push_back(texNormal);
        texFrames.push_back(texMat);
        texFrames.push_back(texRainbow);
        texFrames.push_back(texRedBrick);

        #pragma endregion

        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        float time = 0.0f;
        float timeIncrement = 0.00125f;
        // Main loop.
        while (!window.shouldClose()) {
            window.updateInput();
            imgui();


            if(hierarchicalTransform) {
                glm::mat4 bigSphereTransform = getBigSphereTransform(time);
                glm::mat4 mediumSphereTransform = getMediumSphereTransform(bigSphereTransform, time);
                glm::mat4 smallSphere1Transform = getSmallSphere1Transform(mediumSphereTransform, time);
                glm::mat4 smallSphere2Transform = getSmallSphere2Transform(mediumSphereTransform, time);

                //updatedMesh = meshes[5];
                for (auto& vertex : meshes[6].vertices) {
                        vertex.position = glm::vec3(bigSphereTransform* glm::vec4(vertex.position, 1.0f));
                        vertex.normal = glm::mat3(glm::transpose(glm::inverse(bigSphereTransform))) * vertex.normal; // Correct normal
                }
                //updatedMesh = meshes[3];
                for (auto& vertex : meshes[2].vertices) {
                        vertex.position = glm::vec3(mediumSphereTransform* glm::vec4(vertex.position, 1.0f));
                        vertex.normal = glm::mat3(glm::transpose(glm::inverse(mediumSphereTransform))) * vertex.normal; // Correct normal
                }
                for (auto& vertex : meshes[3].vertices) {
                        vertex.position = glm::vec3(smallSphere1Transform* glm::vec4(vertex.position, 1.0f));
                        vertex.normal = glm::mat3(glm::transpose(glm::inverse(smallSphere1Transform))) * vertex.normal; // Correct normal
                }
                //updatedMesh = meshes[4];
                for (auto& vertex : meshes[4].vertices) {
                        vertex.position = glm::vec3(smallSphere2Transform* glm::vec4(vertex.position, 1.0f));
                        vertex.normal = glm::mat3(glm::transpose(glm::inverse(smallSphere2Transform))) * vertex.normal; // Correct normal
                }

                mesh = mergeMeshes(meshes);

                glBindBuffer(GL_ARRAY_BUFFER, vbo); 
                glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data());

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mesh.triangles.size() * sizeof(glm::uvec3), mesh.triangles.data());
            }

            // Set the active camera
            if (isTopViewCamera) {
                activeCamera = topViewCameraPtr;
            } else if (isRightViewCamera) {
                activeCamera = rightViewCameraPtr;
            } else if (isLeftViewCamera) {
                activeCamera = leftViewCameraPtr;
            } else if (isFrontViewCamera) {
                activeCamera = frontViewCameraPtr;
            } else {
                activeCamera = &mainCamera; // Original camera view set by the toml file
            }



            // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
            glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if(dayNight) {
                
                lights.clear();
                selectedLightIndex = -1;
        
                bool day = false;
                
                glm::mat4 lightTransform = getDayLightTransform(time);
                
                for (auto& vertex : meshes[5].vertices) {
                        vertex.position = glm::vec3(lightTransform * glm::vec4(vertex.position, 1.0f));
                        if(vertex.position.y > 0.0f) {
                            day = true; 
                            posForSun = vertex.position;
                        }
                        vertex.normal = glm::mat3(glm::transpose(glm::inverse(lightTransform))) * vertex.normal; 
                }
                for (auto& vertex : meshes[7].vertices) {
                        vertex.position = glm::vec3(lightTransform * glm::vec4(vertex.position, 1.0f));
                        if(!day) {
                            posForMoon = vertex.position;
                        }
                        vertex.normal = glm::mat3(glm::transpose(glm::inverse(lightTransform))) * vertex.normal; 
                }
                isDay = day;
                if(!day) {
                    if(lights.size() < 1) {
                        moonLight.position = posForMoon;
                        lights.push_back(moonLight);
                        selectedLightIndex = lights.size()-1;
                    }
                    else if(distance(lights[selectedLightIndex].position, posForSun) < 2.0f * rotationSpeed) {
                        lights.erase(lights.begin() + selectedLightIndex);
                        moonLight.position = posForMoon;
                        lights.push_back(moonLight);
                        selectedLightIndex = lights.size()-1;
                    } else {
                        bool alreadyNight = false;
                        for(int i = 0; i < lights.size(); i++) {
                            if(distance(lights[selectedLightIndex].position, posForMoon) < 2.0f * rotationSpeed) {
                                moonLight.position = posForMoon;
                                selectedLightIndex = i;
                                alreadyNight = true;
                                break;
                            }
                        }
                        if(!alreadyNight) {
                            moonLight.position = posForMoon;
                            lights.push_back(moonLight);
                            selectedLightIndex = lights.size()-1;
                        }
                    }
                   
                }
                else {
                    if(lights.size() < 1) {
                        sunLight.position = posForSun;
                        lights.push_back(sunLight);
                    
                        selectedLightIndex = lights.size()-1;
                    }
                    else if(distance(lights[selectedLightIndex].position, posForMoon) < 0.1f) {
                        lights.erase(lights.begin() + selectedLightIndex);
                        sunLight.position = posForSun;
                    lights.push_back(sunLight);
                    
                    selectedLightIndex = lights.size()-1;
                    } else {
                        bool alreadyDay = false;
                        for(int i = 0; i < lights.size(); i++) {
                            if(distance(lights[selectedLightIndex].position, posForSun) < 0.1f) {
                                sunLight.position = posForSun;
                                selectedLightIndex = i;
                                alreadyDay = true;
                                break;
                            }
                        }
                        if(!alreadyDay) {
                            sunLight.position = posForSun;
                            lights.push_back(sunLight);
                            selectedLightIndex = lights.size()-1;
                        }
                    }

                }

                mesh = mergeMeshes(meshes);

                glBindBuffer(GL_ARRAY_BUFFER, vbo); 
                glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data());

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
                glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, mesh.triangles.size() * sizeof(glm::uvec3), mesh.triangles.data());
            }

            // Light projection matrix
            constexpr float fov = glm::pi<float>() / 4.0f;
            constexpr float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
            const glm::mat4 mainProjectionMatrix = glm::perspective(fov, aspect, 0.1f, 30.0f);

            const glm::mat4 lightProjectionMatrix = glm::perspective(glm::pi<float>()/4.0f, 1.0f, 0.1f, 30.0f);

            // Set light projection and view matrix
            glm::mat4 lightViewMatrix = glm::lookAt(
                lights[selectedLightIndex].position,
                lights[selectedLightIndex].position + glm::normalize(lights[selectedLightIndex].direction),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            glm::mat4 lightMVP = mainProjectionMatrix * lightViewMatrix;

            // Set light projection and view matrix for the secondary light
            glm::mat4 secondaryLightViewMatrix = glm::lookAt(
                secondaryLights[selectedSecondaryLightIndex].position,
                secondaryLights[selectedSecondaryLightIndex].position + glm::normalize(secondaryLights[selectedSecondaryLightIndex].direction),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            glm::mat4 secondaryLightMVP = mainProjectionMatrix * secondaryLightViewMatrix;

            // Set model/view/projection matrix.
            const glm::vec3 cameraPos = activeCamera->position();
            const glm::mat4 model { 1.0f };

            const glm::mat4 view = activeCamera->viewMatrix();
            const glm::mat4 projection = activeCamera->projectionMatrix();
            const glm::mat4 mvp = projection * view * model;
            
            if(envMap) {
                glDepthFunc(GL_LEQUAL);
                envMapShader.bind();
                glm::mat4 vieww = glm::mat4(1.0f);
                glm::mat4 projectionn = glm::mat4(1.0f);
                // We make the mat4 into a mat3 and then a mat4 again in order to get rid of the last row and column
                // The last row and column affect the translation of the skybox (which we don't want to affect)
                vieww = glm::mat4(glm::mat3(glm::lookAt(activeCamera->position(), activeCamera->position() + activeCamera->forward(), activeCamera->up())));
		        projectionn = glm::perspective(glm::radians(45.0f), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
                glUniformMatrix4fv(envMapShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(vieww));
                glUniformMatrix4fv(envMapShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projectionn));
                
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
                glUniform1i(envMapShader.getUniformLocation("skybox"), 0);
                glBindVertexArray(skyboxVAO);
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
                glDepthFunc(GL_LESS);
                
                otherEnvMapShader.bind();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
                glUniform1i(otherEnvMapShader.getUniformLocation("skybox"), 0);

                glUniformMatrix4fv(otherEnvMapShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
                glUniformMatrix4fv(otherEnvMapShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(otherEnvMapShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
                
               glUniform3fv(otherEnvMapShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(activeCamera->position()));
               
                // Bind vertex data.
                glBindVertexArray(vao);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
                // Execute draw command.
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
                
                glBindVertexArray(0);
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            }

            else {
            // === Step 1: Render the Shadow Map ===
            {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

                // Clear the shadow map and set needed options
                glClearDepth(1.0);
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                // Bind the shader
                shadowShader.bind();

                // Set viewport size
                glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
                
                glUniformMatrix4fv(shadowShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(lightMVP));

                // Bind vertex data and draw
                glBindVertexArray(vao);
                glVertexAttribPointer(shadowShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

                // Unbind framebuffer
                if (post_process) {
                    glBindFramebuffer(GL_FRAMEBUFFER, postBuffer);
                } else {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
            }

            glClearDepth(1.0);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // === Render the Second Shadow Map ===
            {
                glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer2);

                // Clear the shadow map and set needed options
                glClearDepth(1.0);
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                // Bind the shader
                shadowShader2.bind();

                // Set viewport size
                glViewport(0, 0, SHADOWTEX2_WIDTH, SHADOWTEX2_HEIGHT);
                
                glUniformMatrix4fv(shadowShader2.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(secondaryLightMVP));

                // Bind vertex data and draw
                glBindVertexArray(vao);
                glVertexAttribPointer(shadowShader2.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

                // Unbind framebuffer
                if (post_process) {
                    glBindFramebuffer(GL_FRAMEBUFFER, postBuffer);
                } else {
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                }
            }

            glClearDepth(1.0);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto render = [&](const Shader &shader) {
                // Set the model/view/projection matrix that is used to transform the vertices in the vertex shader.
                glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

                // Bind vertex data.
                glBindVertexArray(vao);

                // We tell OpenGL what each vertex looks like and how they are mapped to the shader using the names
                // NOTE: Usually this can be stored in the VAO, since the locations would be the same in all shaders by using the layout(location = ...) qualifier in the shaders, however this does not work on apple devices.
                glVertexAttribPointer(shader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glVertexAttribPointer(shader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

                // Execute draw command.
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            };

            // Minimap Render Pass
            glBindFramebuffer(GL_FRAMEBUFFER, minimapFBO);
            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 minimapView = topViewCameraPtr->viewMatrix();
            glm::mat4 minimapProjection = topViewCameraPtr->projectionMatrix();
            glm::mat4 minimapModel = glm::mat4(1.0f);
            glm::mat4 minimapMVP = minimapProjection * minimapView * minimapModel;

            minimapShader.bind();
            glUniformMatrix4fv(minimapShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(minimapMVP));
            glUniform3fv(minimapShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
            glUniform3fv(minimapShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
            glUniform3fv(minimapShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
            glVertexAttribPointer(minimapShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glVertexAttribPointer(minimapShader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glBindVertexArray(vao);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);

            if (post_process) {
                glBindFramebuffer(GL_FRAMEBUFFER, postBuffer);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            // Draw mesh into depth buffer but disable color writes.
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            debugShader.bind();
            render(debugShader);

            // Draw the mesh again for each light / shading model.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color writes.
            glDepthMask(GL_FALSE); // Disable depth writes.
            glDepthFunc(GL_EQUAL); // Only draw a pixel if it's depth matches the value stored in the depth buffer.
            glEnable(GL_BLEND); // Enable blending.
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending.
            // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            #pragma region particles

            if (isParticleEffect) {
                // Updating particle values
                const int updateParticleSize = 2;
                for (size_t i = 0; i < updateParticleSize; i++) {
                    int deleteIndex = nextEliminatedParticle(numParticles, particles);
                    setParticleValues(particles[deleteIndex], particleEmitter);
                }
                for (size_t i = 0; i < numParticles; i++) {
                    particles[i].life -= dt;
                    if (particles[i].life > 0.0) { // If particles haven't faded away yet
                        if (isParticleCollision) {
                            for (const auto& obstacle : obstacles) {
                                if (isColliding(particles[i].position, obstacle)) {
                                    handleCollision(particles[i], obstacle);
                                }
                            }
                        }
                        particles[i].position += particles[i].speed * dt;
                        particles[i].color.y += 0.2 * dt;
                    }
                }

                // Particle render
                particleShader.bind();
                glUniformMatrix4fv(particleShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glActiveTexture(GL_TEXTURE0);  
                glBindTexture(GL_TEXTURE_2D, texToon); 
                glUniform1i(xToonShader.getUniformLocation("texToon"), 0);  
                glBindVertexArray(quadVAO);
                for (Particle& particle : particles)
                {
                    glUniform2fv(particleShader.getUniformLocation("offset"), 1, glm::value_ptr(particle.position));
                    glUniform4fv(particleShader.getUniformLocation("particleColor"), 1, glm::value_ptr(particle.color));
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                glBindVertexArray(0);
            }
            #pragma endregion

            // If post process, load the render output to color texture
            if (post_process) {
                glBindFramebuffer(GL_FRAMEBUFFER, postBuffer);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);
            }

            // Animated textures - cycle through textures
            float currTime = glfwGetTime();
            if (currTime - animatedTextureLastFrameTime >= animatedTextureFrameInterval) {
                animatedTextureCurrentFrame = (animatedTextureCurrentFrame + 1) % texFrames.size();
                animatedTextureLastFrameTime = currTime;
            }

            int applyTextureInt = applyTexture ? 1 : 0;

            switch (diffuseMode) {
                case 0: // Debug
                    debugShader.bind();
                    render(debugShader);
                    break;
                case 1: // Lambert Diffuse
                    lambertShader.bind();
                    // === SET YOUR LAMBERT UNIFORMS HERE ===
                    // Values that you may want to pass to the shader include light.position, light.color and shadingData.kd.
                    glUniform3fv(lambertShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(lambertShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(lambertShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    // glUniform1f(lambertShader.getUniformLocation("floatName"), 1, floatValue);
                    // glUniform3fv(lambertShader.getUniformLocation("vecName"), 1, glm::value_ptr(glmVector));
                    render(lambertShader);
                    break;
                case 2: // Toon Lighting Diffuse
                    toonDiffuseShader.bind();
                    // === SET YOUR DIFFUSE TOON UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData.
                    glUniform3fv(toonDiffuseShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(toonDiffuseShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(toonDiffuseShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    glUniform1i(toonDiffuseShader.getUniformLocation("toonDiscretize"), shadingData.toonDiscretize);
                    render(toonDiffuseShader);
                    break;
                case 3: // Toon X Lighting
                    xToonShader.bind();
                    // Bind the light texture to texture slot 2
                    glActiveTexture(GL_TEXTURE0);  
                    glBindTexture(GL_TEXTURE_2D, texToon); 
                    glUniform1i(xToonShader.getUniformLocation("texToon"), 0);  
                    glUniform3fv(xToonShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(xToonShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(xToonShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(xToonShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    glUniform3fv(xToonShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(xToonShader.getUniformLocation("shininess"), shadingData.shininess);
                    glUniform3fv(xToonShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    render(xToonShader);
                    break;
                case 4: // pbr
                    pbrShader.bind();
                    // glUniform1i(pbrShader.getUniformLocation("NUM_LIGHTS"), 3);
                    glUniform3fv(pbrShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(pbrShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(pbrShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(pbrShader.getUniformLocation("albedo"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(pbrShader.getUniformLocation("roughness"), shadingData.roughness);
                    glUniform1f(pbrShader.getUniformLocation("metallic"), shadingData.metallic);
                    glUniform1f(pbrShader.getUniformLocation("intensity"), shadingData.intensity);
                    render(pbrShader);
                    break;
                case 5: // normal mapping
                    normalShader.bind();
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texNormal);
                    glUniform1i(normalShader.getUniformLocation("texNormal"), 0);
                    glActiveTexture(GL_TEXTURE1);
                    // Normal Mapping Texture
                    glBindTexture(GL_TEXTURE_2D, texToon);
                    glUniform1i(normalShader.getUniformLocation("texMat"), 1);
                    glUniform3fv(normalShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    // glUniform3fv(normalShader.getUniformLocation("viewPos"), 1, glm::value_ptr(cameraPos));
                    // glUniform3fv(normalShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    // glUniform3fv(normalShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniformMatrix4fv(normalShader.getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
                    glUniformMatrix4fv(normalShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
                    glUniformMatrix4fv(normalShader.getUniformLocation("proj"), 1, GL_FALSE, glm::value_ptr(projection));
                    // Bind vertex data.
                    glBindVertexArray(vao);
                    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
                    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
                    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
                    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
                    // Enable the tangent and bitangent attributes
                    glEnableVertexAttribArray(2);
                    glEnableVertexAttribArray(3);
                    glEnableVertexAttribArray(4);
                        
                    // Execute draw command.
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

                    glBindVertexArray(0);
                    break;
                default: // Debug mode as default
                    debugShader.bind();
                    render(debugShader);
                    break;
            }

            switch (specularMode) {
                case 0: // None
                    // do nothing!!! (no specular lighting)
                    break;
                case 1: // Phong Specular Lighting
                    phongShader.bind();
                    // === SET YOUR PHONG/BLINN PHONG UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                    glUniform3fv(phongShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(phongShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(phongShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(phongShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(phongShader.getUniformLocation("shininess"), shadingData.shininess);
                    render(phongShader);
                    break;
                case 2: // Blinn-Phong Specular Lighting
                    blinnPhongShader.bind();
                    // === SET YOUR PHONG/BLINN PHONG UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                    glUniform3fv(blinnPhongShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(blinnPhongShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(blinnPhongShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(blinnPhongShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(blinnPhongShader.getUniformLocation("shininess"), shadingData.shininess);
                    render(blinnPhongShader);
                    break;
                case 3: // Toon Lighting Specular
                    toonSpecularShader.bind();
                    // === SET YOUR SPECULAR TOON UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                    glUniform3fv(toonSpecularShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(toonSpecularShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform1f(toonSpecularShader.getUniformLocation("shininess"), shadingData.shininess);
                    glUniform1f(toonSpecularShader.getUniformLocation("toonSpecularThreshold"), shadingData.toonSpecularThreshold);
                    glUniform3fv(toonSpecularShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform3fv(toonSpecularShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    render(toonSpecularShader);
                    break;
                default: // None as default
                    // do nothing!!! (no specular lighting)
                    break;
            }

            if (shadows || pcf || lights[selectedLightIndex].is_spotlight) {
                mainShader.bind();
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texShadow);
                glUniform1i(mainShader.getUniformLocation("texShadow"), 1);
                int isSpotlightInt = lights[selectedLightIndex].is_spotlight ? 1 : 0;
                int shadowsInt = shadows ? 1 : 0;
                int pcfInt = pcf ? 1 : 0;
                glUniform1i(mainShader.getUniformLocation("isSpotlight"), isSpotlightInt);
                glUniform1i(mainShader.getUniformLocation("shadows"), shadowsInt);
                glUniform1i(mainShader.getUniformLocation("pcf"), pcfInt);
                glUniformMatrix4fv(mainShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(mainShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                glUniform3fv(mainShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                render(mainShader);
            }

            if (multipleShadows) {
                secondaryMainShader.bind();
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, shadowTex2);
                glUniform1i(secondaryMainShader.getUniformLocation("shadowTex2"), 2);
                int isSpotlightInt = secondaryLights[selectedSecondaryLightIndex].is_spotlight ? 1 : 0;
                int shadowsInt = shadows ? 1 : 0;
                int pcfInt = pcf ? 1 : 0;
                glUniform1i(secondaryMainShader.getUniformLocation("isSpotlight"), isSpotlightInt);
                glUniform1i(secondaryMainShader.getUniformLocation("shadows"), shadowsInt);
                glUniform1i(secondaryMainShader.getUniformLocation("pcf"), pcfInt);
                glUniformMatrix4fv(secondaryMainShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(secondaryMainShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(secondaryLightMVP));
                glUniform3fv(secondaryMainShader.getUniformLocation("lightPos"), 1, glm::value_ptr(secondaryLights[selectedSecondaryLightIndex].position));
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                render(secondaryMainShader);
            }

            if (lights[selectedLightIndex].has_texture) {
                lightTextureShader.bind();
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texFrames[animatedTextureCurrentFrame]);
                glUniform1i(lightTextureShader.getUniformLocation("texLight"), 2);
                int applyTextureInt = lights[selectedLightIndex].has_texture ? 1 : 0;
                glUniform1i(lightTextureShader.getUniformLocation("applyTexture"), applyTextureInt);
                glUniformMatrix4fv(lightTextureShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(lightTextureShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                glUniform3fv(lightTextureShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                render(lightTextureShader);
            }

            if (applyMinimap) {
                mapShader.bind();
                glActiveTexture(GL_TEXTURE3);
                glBindTexture(GL_TEXTURE_2D, minimapTexture);
                glUniform1i(mapShader.getUniformLocation("texMinimap"), 3);
                glUniformMatrix4fv(mapShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniform3fv(mapShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                render(mapShader);
            }

            // Implement smooth path along the Bezier Curve for light movement
            if (applySmoothPath) {
                float currentTime = glfwGetTime();
                static float lastFrameTime = 0.0f;
                float timeChange = currentTime - lastFrameTime;
                lastFrameTime = currentTime;

                if (isConstantSpeedAlongBezier) moveLightAlongEvenlySpacedPath(timeChange);
                else changeLightPosAlongBezierCurves(timeChange);
            }

            // Implement smooth path along the Bezier Curve for camera movement
            if (moveCamera) {
                float currentTime = glfwGetTime();
                static float lastFrameTime = 0.0f;
                float timeChange = currentTime - lastFrameTime;
                lastFrameTime = currentTime;

                float speedFactor = 0.1;
                t_camera += timeChange * speedFactor;

                if (t_camera > 1.0) {
                    t_camera = 0.0;
                }

                const glm::vec3 newCameraPos = computeBezierPoint(
                    t_camera, 
                    middlePlaneBezierCurve[0], 
                    middlePlaneBezierCurve[1], 
                    middlePlaneBezierCurve[2], 
                    middlePlaneBezierCurve[3]
                );

                activeCamera->setLookAt(newCameraPos);
            } else {
                activeCamera->setLookAt(look_at); // Set the camera position back at its original place
            }

            // Restore default depth test settings and disable blending.
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            
            if (post_process) {
                glDisable(GL_DEPTH_TEST);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, WIDTH, HEIGHT);
                screenShader.bind();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texScreen);
                glUniform1i(screenShader.getUniformLocation("screenTexture"), 0);

                glBindVertexArray(quadVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindVertexArray(0);
            }
            
            }
            if(time <= 0.1f&&time>=0.05) time -= timeIncrement;
            else if(time < 0.05)
            time += timeIncrement;
            //time = time/50.0f;
            else time = 0.1f;
            window.swapBuffers();
        }

        // Be a nice citizen and clean up after yourself.
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texShadow);
        glDeleteTextures(1, &texLight);
        glDeleteTextures(1, &texNormal);
        glDeleteTextures(1, &texToon);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);
        glDeleteVertexArrays(1, &vao);
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);
        glDeleteBuffers(1, &skyboxVBO);
        glDeleteBuffers(1, &skyboxVAO);
        glDeleteBuffers(1, &skyboxIBO);
    } 
    //#pragma endregion
    //#pragma region Animation
    else { // if animated
        
        animationMeshes = loadMeshesFromDirectory(mesh_path);
        std::cout << animationMeshes.size() << std::endl;

        // Initialize the initial mesh object
        const Mesh mesh = animationMeshes[0];
        frameDuration = 1.0f / static_cast<float>(animationMeshes.size());

        window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
            if (key == '\\' && action == GLFW_PRESS) {
                show_imgui = !show_imgui;
            }

            if (action != GLFW_RELEASE)
                return;

            const bool shiftPressed = window.isKeyPressed(GLFW_KEY_LEFT_SHIFT) || window.isKeyPressed(GLFW_KEY_RIGHT_SHIFT);

            switch (key) {
            case GLFW_KEY_L: {
                if (shiftPressed)
                    lights.push_back(Light { activeCamera->position(), glm::vec3(1) });
                else
                    lights[selectedLightIndex].position = activeCamera->position();
                    lights[selectedLightIndex].direction = activeCamera->forward();
                return;
            }
            case GLFW_KEY_N: {
                resetLights();
                return;
            }
            case GLFW_KEY_T: {
                if (shiftPressed)
                    shadingData.toonSpecularThreshold += 0.001f;
                else
                    shadingData.toonSpecularThreshold -= 0.001f;
                std::cout << "ToonSpecular: " << shadingData.toonSpecularThreshold << std::endl;
                return;
            }
            case GLFW_KEY_D: {
                if (shiftPressed) {
                    ++shadingData.toonDiscretize;
                } else {
                    if (--shadingData.toonDiscretize < 1)
                        shadingData.toonDiscretize = 1;
                }
                std::cout << "Toon Discretization levels: " << shadingData.toonDiscretize << std::endl;
                return;
            }
            default:
                return;
            };
        });

        // Shading functionality
        const Shader debugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl").build();
        const Shader lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl").build();
        const Shader lambertShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lambert_frag.glsl").build();
        const Shader phongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/phong_frag.glsl").build();
        const Shader blinnPhongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_phong_frag.glsl").build();
        const Shader toonDiffuseShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_diffuse_frag.glsl").build();
        const Shader toonSpecularShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_specular_frag.glsl").build();
        const Shader xToonShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/xtoon_frag.glsl").build();

        const Shader mainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").build();
        const Shader shadowShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").build();
        const Shader lightTextureShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_texture_frag.glsl").build();

        // Create Vertex Buffer Object and Index Buffer Objects.
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        GLuint ibo;
        // Create index buffer object (IBO)
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Bind vertex data to shader inputs using their index (location).
        // These bindings are stored in the Vertex Array Object.
        GLuint vao;
        // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

        // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
        // The stride is the distance in bytes between vertices. We use the offset to point to the normals
        // instead of the positions.
        // Tell OpenGL that we will be using vertex attributes 0 and 1.
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        // This is where we would set the attribute pointers, if apple supported it.

        glBindVertexArray(0);

        // Load image from disk to CPU memory.
        int width, height, sourceNumChannels; // Number of channels in source image. pixels will always be the requested number of channels (3).
        stbi_uc* pixels = stbi_load(RESOURCE_ROOT "resources/toon_map.png", &width, &height, &sourceNumChannels, STBI_rgb);

        // Create a texture on the GPU with 3 channels with 8 bits each.
        GLuint texToon;
        glGenTextures(1, &texToon);
        glBindTexture(GL_TEXTURE_2D, texToon);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

        // Set behavior for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // Create Light Texture
        int texWidth, texHeight, texChannels;
        auto light_texture_path = std::string(RESOURCE_ROOT) + config["lights"]["texture_path"][0].value_or("resources/smiley.png");
        stbi_uc* light_pixels = stbi_load(light_texture_path.c_str(), &texWidth, &texHeight, &texChannels, 3); 
        //stbi_uc* light_pixels = stbi_load(RESOURCE_ROOT "resources/smiley.png", &texWidth, &texHeight, &texChannels, 3);

        GLuint texLight;
        glGenTextures(1, &texLight);
        glBindTexture(GL_TEXTURE_2D, texLight);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, light_pixels);

        // Set behaviour for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        // === Create Shadow Texture ===
        GLuint texShadow;
        const int SHADOWTEX_WIDTH = 1024;
        const int SHADOWTEX_HEIGHT = 1024;
        glGenTextures(1, &texShadow);
        glBindTexture(GL_TEXTURE_2D, texShadow);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        // Set behaviour for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glBindTexture(GL_TEXTURE_2D, 0);
        
        // === Create framebuffer for extra texture ===
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texShadow, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Free the CPU memory after we copied the image to the GPU.
        stbi_image_free(pixels);
        stbi_image_free(light_pixels);
        
        // Enable depth testing.
        glEnable(GL_DEPTH_TEST);

        // Main loop.
        while (!window.shouldClose()) {
            float currentTime = static_cast<float>(glfwGetTime());  
            float deltaTime = currentTime - lastFrameTime;  
            lastFrameTime = currentTime; 

            window.updateInput();

            imgui();

            timeAccumulator += deltaTime;

            if (timeAccumulator >= frameDuration) {
                // Move to the next mesh in the loop
                currentMeshIndex = (currentMeshIndex + 1) % animationMeshes.size();
                timeAccumulator = 0.0f;  // Reset the accumulator for the next frame
            }

            // Get the current mesh for rendering
            const Mesh& mesh = animationMeshes[currentMeshIndex];

            // Create Vertex Buffer Object and Index Buffer Objects.
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Create index buffer object (IBO)
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            // Bind vertex data to shader inputs using their index (location).
            // These bindings are stored in the Vertex Array Object.
            // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

            // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
            // The stride is the distance in bytes between vertices. We use the offset to point to the normals
            // instead of the positions.
            // Tell OpenGL that we will be using vertex attributes 0 and 1.
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            // This is where we would set the attribute pointers, if apple supported it.

            glBindVertexArray(0);

            // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
            glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Light projection matrix
            constexpr float fov = glm::pi<float>() / 4.0f;
            constexpr float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
            const glm::mat4 mainProjectionMatrix = glm::perspective(fov, aspect, 0.1f, 30.0f);

            const glm::mat4 lightProjectionMatrix = glm::perspective(glm::pi<float>()/4.0f, 1.0f, 0.1f, 30.0f);

            // Set light projection and view matrix
            glm::mat4 lightViewMatrix = glm::lookAt(
                lights[selectedLightIndex].position,
                lights[selectedLightIndex].position + glm::normalize(lights[selectedLightIndex].direction),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            glm::mat4 lightMVP = mainProjectionMatrix * lightViewMatrix;

            // Set model/view/projection matrix.
            const glm::vec3 cameraPos = activeCamera->position();
            const glm::mat4 model { 1.0f };

            const glm::mat4 view = activeCamera->viewMatrix();
            const glm::mat4 projection = activeCamera->projectionMatrix();
            const glm::mat4 mvp = projection * view * model;

            // === Step 1: Render the Shadow Map ===
            {
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

                // Clear the shadow map and set needed options
                glClearDepth(1.0);
                glClear(GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                // Bind the shader
                shadowShader.bind();

                // Set viewport size
                glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
                
                glUniformMatrix4fv(shadowShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(lightMVP));

                // Bind vertex data and draw
                glBindVertexArray(vao);
                glVertexAttribPointer(shadowShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

                // Unbind framebuffer
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            glClearDepth(1.0);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto render = [&](const Shader &shader) {
                // Set the model/view/projection matrix that is used to transform the vertices in the vertex shader.
                glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

                // Bind vertex data.
                glBindVertexArray(vao);

                // We tell OpenGL what each vertex looks like and how they are mapped to the shader using the names
                // NOTE: Usually this can be stored in the VAO, since the locations would be the same in all shaders by using the layout(location = ...) qualifier in the shaders, however this does not work on apple devices.
                glVertexAttribPointer(shader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
                glVertexAttribPointer(shader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

                // Execute draw command.
                glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

                glBindVertexArray(0);
            };
            
            // Draw mesh into depth buffer but disable color writes.
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            debugShader.bind();
            render(debugShader);

            // Draw the mesh again for each light / shading model.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color writes.
            glDepthMask(GL_FALSE); // Disable depth writes.
            glDepthFunc(GL_EQUAL); // Only draw a pixel if it's depth matches the value stored in the depth buffer.
            glEnable(GL_BLEND); // Enable blending.
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending.

            /*
            std::array diffuseModes {"Debug", "Lambert Diffuse", "Toon Lighting Diffuse", "Toon X Lighting"};
            std::array specularModes {"None", "Phong Specular Lighting", "Blinn-Phong Specular Lighting", "Toon Lighting Specular"};
            */

            //for (const Light& light : lights) {
                //renderedSomething = false;

            int applyTextureInt = applyTexture ? 1 : 0;
            switch (diffuseMode) {
                case 0: // Debug
                    debugShader.bind();
                    render(debugShader);
                    break;
                case 1: // Lambert Diffuse
                    lambertShader.bind();
                    // === SET YOUR LAMBERT UNIFORMS HERE ===
                    // Values that you may want to pass to the shader include light.position, light.color and shadingData.kd.
                    glUniform3fv(lambertShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(lambertShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(lambertShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    // glUniform1f(lambertShader.getUniformLocation("floatName"), 1, floatValue);
                    // glUniform3fv(lambertShader.getUniformLocation("vecName"), 1, glm::value_ptr(glmVector));
                    render(lambertShader);
                    break;
                case 2: // Toon Lighting Diffuse
                    toonDiffuseShader.bind();
                    // === SET YOUR DIFFUSE TOON UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData.
                    glUniform3fv(toonDiffuseShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(toonDiffuseShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(toonDiffuseShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    glUniform1i(toonDiffuseShader.getUniformLocation("toonDiscretize"), shadingData.toonDiscretize);
                    render(toonDiffuseShader);
                    break;
                case 3: // Toon X Lighting
                    xToonShader.bind();
                    // Bind the light texture to texture slot 2
                    glActiveTexture(GL_TEXTURE0);  
                    glBindTexture(GL_TEXTURE_2D, texToon); 
                    glUniform1i(xToonShader.getUniformLocation("texToon"), 0);  
                    glUniform3fv(xToonShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(xToonShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(xToonShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(xToonShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    glUniform3fv(xToonShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(xToonShader.getUniformLocation("shininess"), shadingData.shininess);
                    glUniform3fv(xToonShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    render(xToonShader);
                    break;
                default: // Debug mode as default
                    debugShader.bind();
                    render(debugShader);
                    break;
            }

            switch (specularMode) {
                case 0: // None
                    // do nothing!!! (no specular lighting)
                    break;
                case 1: // Phong Specular Lighting
                    phongShader.bind();
                    // === SET YOUR PHONG/BLINN PHONG UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                    glUniform3fv(phongShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(phongShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(phongShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(phongShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(phongShader.getUniformLocation("shininess"), shadingData.shininess);
                    render(phongShader);
                    break;
                case 2: // Blinn-Phong Specular Lighting
                    blinnPhongShader.bind();
                    // === SET YOUR PHONG/BLINN PHONG UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                    glUniform3fv(blinnPhongShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(blinnPhongShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(blinnPhongShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(blinnPhongShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(blinnPhongShader.getUniformLocation("shininess"), shadingData.shininess);
                    render(blinnPhongShader);
                    break;
                case 3: // Toon Lighting Specular
                    toonSpecularShader.bind();
                    // === SET YOUR SPECULAR TOON UNIFORMS HERE ===
                    // Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
                    glUniform3fv(toonSpecularShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(toonSpecularShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform1f(toonSpecularShader.getUniformLocation("shininess"), shadingData.shininess);
                    glUniform1f(toonSpecularShader.getUniformLocation("toonSpecularThreshold"), shadingData.toonSpecularThreshold);
                    glUniform3fv(toonSpecularShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
                    glUniform3fv(toonSpecularShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    render(toonSpecularShader);
                    break;
                default: // None as default
                    // do nothing!!! (no specular lighting)
                    break;
            }

            if (shadows || pcf || lights[selectedLightIndex].is_spotlight) {
                mainShader.bind();
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, texShadow);
                glUniform1i(mainShader.getUniformLocation("texShadow"), 1);
                int isSpotlightInt = lights[selectedLightIndex].is_spotlight ? 1 : 0;
                int shadowsInt = shadows ? 1 : 0;
                int pcfInt = pcf ? 1 : 0;
                glUniform1i(mainShader.getUniformLocation("isSpotlight"), isSpotlightInt);
                glUniform1i(mainShader.getUniformLocation("shadows"), shadowsInt);
                glUniform1i(mainShader.getUniformLocation("pcf"), pcfInt);
                glUniformMatrix4fv(mainShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(mainShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                glUniform3fv(mainShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                render(mainShader);
            }

            if (lights[selectedLightIndex].has_texture) {
                lightTextureShader.bind();
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, texLight);
                glUniform1i(lightTextureShader.getUniformLocation("texLight"), 2);
                int applyTextureInt = applyTexture ? 1 : 0;
                glUniform1i(lightTextureShader.getUniformLocation("applyTexture"), applyTextureInt);
                glUniformMatrix4fv(lightTextureShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
                glUniformMatrix4fv(lightTextureShader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                glUniform3fv(lightTextureShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                render(lightTextureShader);
            }

            // Restore default depth test settings and disable blending.
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);

            // Present result to the screen.
            window.swapBuffers();
        }

        // Be a nice citizen and clean up after yourself.
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &texShadow);
        glDeleteTextures(1, &texLight);
        glDeleteTextures(1, &texToon);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);
        glDeleteVertexArrays(1, &vao);
    }
    //#pragma endregion
    
    return 0;
}
//#pragma endregion





