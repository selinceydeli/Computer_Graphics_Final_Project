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

std::vector<Mesh> animationMeshes; 
std::vector<GLuint> vaos, vbos, ibos;
float frameDuration = 0.0f;  
float timeAccumulator = 0.0f;
float lastFrameTime = 0.0f;
size_t currentMeshIndex = 0;   

// Configuration
const int WIDTH = 1200;
const int HEIGHT = 800;

bool show_imgui = true;

/*
bool debug = true;
bool diffuseLighting = false;
bool phongSpecularLighting = false;
bool blinnPhongSpecularLighting = false;
bool toonLightingDiffuse = false;
bool toonLightingSpecular = false;
bool toonxLighting = false;
*/

std::array diffuseModes {"Debug", "Lambert Diffuse", "Toon Lighting Diffuse", "Toon X Lighting", "PBR Shading"};
std::array specularModes {"None", "Phong Specular Lighting", "Blinn-Phong Specular Lighting", "Toon Lighting Specular"};

std::array samplingModes {"Single Sample", "PCF"};
bool shadows = false;
bool pcf = false;
bool applyTexture = false;

int samplingMode = 0;
int diffuseMode = 0;
int specularMode = 0;

bool transparency = true;

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
};

std::vector<Light> lights {};
size_t selectedLightIndex = 0;

void resetLights()
{
    lights.clear();
    lights.push_back(Light { glm::vec3(0, 0, 3), glm::vec3(1) });
    selectedLightIndex = 0;
}

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

    /*
    ImGui::Separator();
    ImGui::Text("Diffuse Model");
    ImGui::Checkbox("0: Debug", &debug);
    ImGui::Checkbox("1: Diffuse Lighting", &diffuseLighting);
    ImGui::Checkbox("2: Toon Lighting Diffuse", &toonLightingDiffuse);
    ImGui::Checkbox("3: Toon X Lighting", &toonxLighting);

    ImGui::Separator();
    ImGui::Text("Specular Model");
    ImGui::Checkbox("4: Phong Specular Lighting", &phongSpecularLighting);
    ImGui::Checkbox("5: Blinn-Phong Specular Lighting", &blinnPhongSpecularLighting);
    ImGui::Checkbox("6: Toon Lighting Specular", &toonLightingSpecular);
    */

    ImGui::Separator();
    ImGui::Combo("Diffuse Mode", &diffuseMode, diffuseModes.data(), (int)diffuseModes.size());

    ImGui::Separator();
    ImGui::Combo("Specular Mode", &specularMode, specularModes.data(), (int)specularModes.size());

    ImGui::Separator();
    ImGui::Checkbox("Enable Texture to Light", &lights[selectedLightIndex].has_texture);

    ImGui::Separator();
    ImGui::Checkbox("Enable Shadows", &shadows);

    ImGui::Separator();
    ImGui::Checkbox("Enable PCF", &pcf);

    ImGui::Separator();
    ImGui::Text("Lights");

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

// Program entry point. Everything starts here.
int main(int argc, char** argv)
{
    // read toml file from argument line (otherwise use default file)
    //std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/checkout.toml";
    std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/pbr_test.toml"; // Scene for animation

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
    shadingData.toonDiscretize = (int) config["material"]["toonDiscretize"].value_or(0);
    shadingData.toonSpecularThreshold = config["material"]["toonSpecularThreshold"].value_or(0.0f);

    // read lights
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
    } else {
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

    Trackball trackball { &window, glm::radians(fovY) };
    trackball.setCamera(look_at, rotations, dist);

    // read mesh
    bool animated = config["mesh"]["animated"].value_or(false);
    auto mesh_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
    std::cout << mesh_path << std::endl;

    #pragma region Render
    if (!animated) {
        //const Mesh mesh = loadMesh(mesh_path)[0];
        //const Mesh mesh = mergeMeshes(loadMesh(mesh_path));
        //const Mesh mesh = mergeMeshes(loadMesh(RESOURCE_ROOT "build/resources/scene.obj"));
        const Mesh mesh = mergeMeshes(loadMesh(mesh_path));

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
                    lights.push_back(Light { trackball.position(), glm::vec3(1) });
                else
                    lights[selectedLightIndex].position = trackball.position();
                    lights[selectedLightIndex].direction = trackball.forward();
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
        const Shader pbrShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/pbr_frag.glsl").build();
        
        const Shader masterShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/master_shader.glsl").build();

        const Shader mainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").build();
        const Shader shadowShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").build();

        const Shader lightTextureShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_texture_frag.glsl").build();
        //const Shader spotlightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/spotlight_frag.glsl").build();
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
            window.updateInput();

            imgui();

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
            const glm::vec3 cameraPos = trackball.position();
            const glm::mat4 model { 1.0f };

            const glm::mat4 view = trackball.viewMatrix();
            const glm::mat4 projection = trackball.projectionMatrix();
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
                case 4: // pbr
                    pbrShader.bind();
                    glUniform3fv(pbrShader.getUniformLocation("lightPos"), 1, glm::value_ptr(lights[selectedLightIndex].position));
                    glUniform3fv(pbrShader.getUniformLocation("lightColor"), 1, glm::value_ptr(lights[selectedLightIndex].color));
                    glUniform3fv(pbrShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
                    glUniform3fv(pbrShader.getUniformLocation("albedo"), 1, glm::value_ptr(shadingData.ks));
                    glUniform1f(pbrShader.getUniformLocation("roughness"), shadingData.roughness);
                    glUniform1f(pbrShader.getUniformLocation("metallic"), shadingData.metallic);
                    render(pbrShader);
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
                int applyTextureInt = lights[selectedLightIndex].has_texture ? 1 : 0;
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
    #pragma endregion
    #pragma region Animation
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
                    lights.push_back(Light { trackball.position(), glm::vec3(1) });
                else
                    lights[selectedLightIndex].position = trackball.position();
                    lights[selectedLightIndex].direction = trackball.forward();
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

        const Shader masterShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/master_shader.glsl").build();

        const Shader mainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").build();
        const Shader shadowShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").build();

        const Shader lightTextureShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_texture_frag.glsl").build();
        //const Shader spotlightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/spotlight_frag.glsl").build();

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
            const glm::vec3 cameraPos = trackball.position();
            const glm::mat4 model { 1.0f };

            const glm::mat4 view = trackball.viewMatrix();
            const glm::mat4 projection = trackball.projectionMatrix();
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
    #pragma endregion
    
    return 0;
}
