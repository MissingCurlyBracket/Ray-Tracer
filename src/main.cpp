#include "bounding_volume_hierarchy.h"
#include "draw.h"
#include "ray_tracing.h"
#include "screen.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <nfd.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
DISABLE_WARNINGS_POP()
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <framework/image.h>
#include <framework/imguizmo.h>
#include <framework/trackball.h>
#include <framework/variant_helper.h>
#include <framework/window.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <type_traits>
#include <variant>

constexpr glm::ivec2 windowResolution { 800, 800 };
const std::filesystem::path dataPath { DATA_DIR };
bool blur = false;

enum class ViewMode {
    Rasterization = 0,
    RayTracing = 1
};

static glm::vec3 getFinalColor(const Scene& scene, const BoundingVolumeHierarchy& bvh, Ray ray, int recursion);

static glm::vec3 calculatePhongShading(const Ray ray, const PointLight& light, HitInfo& hitInfo, const Scene& scene, const BoundingVolumeHierarchy& bvh, int recursion) {
    glm::vec3 reflectivity = hitInfo.material.ks;
    glm::vec3 vertexPos = ray.origin + ray.t * ray.direction;
    glm::vec3 lightVector = glm::normalize(light.position - vertexPos);
    glm::vec3 normal = (glm::dot(lightVector, hitInfo.normal) < 0) ? -1.0f * hitInfo.normal : hitInfo.normal;
    //debug normal ray
    drawRay({ vertexPos, normal, 0.1 }, glm::vec3 (1,1,0));

    //Diffuse    
    glm::vec3 diffuse = light.color * hitInfo.material.kd * glm::max(0.0f, glm::dot(lightVector, normal));

    //Specular
    glm::vec3 viewVector = glm::normalize(ray.origin - vertexPos);
    glm::vec3 reflectionVector = glm::normalize((2.0f * normal * glm::dot(normal, lightVector)) - lightVector);
    //debug reflection ray
    //drawRay({ vertexPos, {2.0f * normal * glm::dot(normal, viewVector) - viewVector}, 0.5 }, glm::vec3(1, 0, 1));
    glm::vec3 specular = light.color * hitInfo.material.ks * (glm::pow(glm::max(0.0f, glm::dot(reflectionVector, viewVector)), hitInfo.material.shininess));

    glm::vec3 phong = (glm::dot(viewVector, normal) < 0.0f && glm::dot(lightVector, normal) > 0.0f) ? glm::vec3(0) : diffuse + specular;

    
    //Hard Shadows - Works

    //the shadow ray which starts at the point on the mesh and goes towards the light
    Ray shadowRay;
    shadowRay.direction = glm::normalize(light.position - vertexPos);
    shadowRay.origin = vertexPos + 0.0001f * shadowRay.direction; //small offset to avoid self shadowing

    //when the shadow ray intersects something
    if (bvh.intersect(shadowRay, hitInfo)) {
        glm::vec3 nextIntersect = shadowRay.origin + (shadowRay.direction * shadowRay.t) - vertexPos; //vector from the vertex to the first intersection of the shadow ray
        //if the shadow ray intersects something before reaching the light, then the vertex is in shadow
        if (glm::length(light.position - vertexPos) > glm::length(nextIntersect)) {
            //red debug shadow ray drawn when point is in shadow
            phong = glm::vec3{ 0.0f };
            drawRay(shadowRay, glm::vec3{ 1.0f, 0.0f, 0.0f });
        }
        else {
            //if the lights is before the intersection then draw light ray to light
            drawRay({ vertexPos, light.position - vertexPos, 1.0f }, light.color);
        }
    }
    else {
        //if there is no intersection at all, draw light ray to light
        drawRay({ vertexPos, light.position - vertexPos, 1.0f }, light.color);
    }


    // Regular Reflections - Works
    if (reflectivity != glm::vec3(0) && recursion > 0) {
        //the reflection vector to the view vector
        glm::vec3 reflection = 2.0f * normal * glm::dot(normal, viewVector) - viewVector;
        Ray reflectedRay = { vertexPos + (0.0001f * reflection), reflection,  std::numeric_limits<float>::max() };
        //the shading will be the same shading as what the reflected ray would have
        phong = phong + reflectivity * getFinalColor(scene, bvh, reflectedRay, recursion - 1);
    }


    // Glossy Reflection - works
    // Replaces the regular reflection that is commented out between lines 92 and 101. 
    // If you want to test reflection alone comment out the if statement below and uncomment the if statement above marked as "reflection"
    // Takes account the shininess when calculating the reflection to give it a glossy reflection look.
    // Check that shininess is above 0 on top of the other checks of reflection to avoid dividing by zero inside the loop
//    if (reflectivity != glm::vec3(0) && recursion > 0 && hitInfo.material.shininess > 0) {
//        // All the vectors and rays I will be using within the for loop.
//        glm::vec3 reflection = 2.0f * normal * glm::dot(normal, viewVector) - viewVector;
//        glm::vec3 direction;
//        glm::vec3 glossy;
//        glm::vec3 glossyColor = { 0,0,0 };
//        Ray glossyRay;
//
//        // Count of how many of the iterations actually create a random vector that is above the surface.
//        int count = 80;
//
//        for (int i = 0; i < 80; i++) {
//
//            // create random floats between -1 and 1
//            float xDir = ((2 * (double)std::rand() / RAND_MAX) - 1) / 2;
//            float yDir = ((2 * (double)std::rand() / RAND_MAX) - 1) / 2;
//            float zDir = ((2 * (double)std::rand() / RAND_MAX) - 1) / 2;
//
//            // Create a direction vector with random x, y, z floats divided by the square of shininess to emphasize the effect
//            // of shininess on how far range can the random vectors be from the regular reflection vector
//            direction = { xDir / (hitInfo.material.shininess), yDir / (hitInfo.material.shininess), zDir / (hitInfo.material.shininess) };
//            // add the deflected vector to reflection to create the r", the new glossy reflection vector and normalize it
//            glossy = reflection + direction;
//            glossy = glm::normalize(glossy);
//
//            // Check if the new vector is above the surface, to make sure we don't get random black points
//            // don't include these vectors in the color calculation and reduce count to make the averaging correctly as well
//            if (std::abs(std::acos(glm::dot(glossy, normal))) >= 90) {
//                count = count - 1;
//                continue;
//            }
//            // If the vector is above the surface create a ray from the vector with a slight epsilon of 0.0001 to make sure that we get correct colors
//            // sum up colors for all the iterations
//            else {
//                glossyRay = { vertexPos + (0.0001f * glossy), glossy,  std::numeric_limits<float>::max() };
//                glossyColor += getFinalColor(scene, bvh, glossyRay, recursion - 1);
//            }
//
//        }
//
//        //the shading will be the same shading as what the reflected ray would have been but the color is replaced by the new glossy color
//        // the color is divided by the count to make sure that the ignored iterations aren't calculated in the average
//        phong = phong + glm::vec3{ glossyColor.x / count, glossyColor.y / count, glossyColor.z / count };
//    }

    return phong;
}

static glm::vec3 getFinalColor(const Scene& scene, const BoundingVolumeHierarchy& bvh, Ray ray, int recursion) {
    HitInfo hitInfo;
    if (bvh.intersect(ray, hitInfo)) {
        glm::vec3 color = glm::vec3(0);
        glm::vec3 vertexPos = ray.origin + ray.t * ray.direction;
        for (const auto& light : scene.lights) {
            if (std::holds_alternative<PointLight>(light)) {
                const PointLight pointLight = std::get<PointLight>(light);
                
                color += calculatePhongShading(ray, pointLight, hitInfo, scene, bvh, recursion);
            }
            else if (std::holds_alternative<SegmentLight>(light)) {
                const SegmentLight segmentLight = std::get<SegmentLight>(light);
                float lengthOfSegLight = glm::length(segmentLight.endpoint0 - segmentLight.endpoint1);
                //if the light is 1 unit long then there will be 10 lights on it
                int numLights = glm::floor(lengthOfSegLight * 10);
                glm::vec3 segmentVector = segmentLight.endpoint1 - segmentLight.endpoint0;

                //loop over to generate point lights along the segment light
                for (int i = 0; i <= numLights; i++) {
                    //the position of the light will be a fraction of the segment light length defined by the number of lights.
                    glm::vec3 currentPos = segmentLight.endpoint0 + (lengthOfSegLight / numLights) * i * glm::normalize(segmentVector);
                    
                    //weights of each endpoint of the segment light
                    float lengthOfAlpha = glm::length(currentPos - segmentLight.endpoint0);
                    float lengthOfBeta = lengthOfSegLight - lengthOfAlpha;
                    glm::vec3 currentPosColor = ((lengthOfAlpha * segmentLight.color1 / lengthOfSegLight) + (lengthOfBeta * segmentLight.color0 / lengthOfSegLight)) / 10.0f;
                    
                    //drawRay({ currentPos, currentPos - vertexPos, 1.0f }, currentPosColor);

                    const PointLight pointLightCurrent = { currentPos, currentPosColor };
                    //calculate phong for each of the point lights
                    color += calculatePhongShading(ray, pointLightCurrent, hitInfo, scene, bvh, recursion);
                }
            }
            else if (std::holds_alternative<ParallelogramLight>(light)) {
                const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);
                
                glm::vec3 v1 = parallelogramLight.edge01 + parallelogramLight.v0;
                glm::vec3 v2 = parallelogramLight.edge02 + parallelogramLight.v0;
                glm::vec3 v3 = parallelogramLight.edge01 + parallelogramLight.edge01 + parallelogramLight.v0;

                float edge01Length = glm::length(parallelogramLight.edge01);
                float edge02Length = glm::length(parallelogramLight.edge02);
                //for every unit of length on both edges, there will be 20 generated lights
                int numLightsEdge01 = glm::floor(edge01Length * 20);
                int numLightsEdge02 = glm::floor(edge02Length * 20);
                
                //generate the point lights one column at a time
                for (int i = 0; i <= numLightsEdge01; i++) {
                    for (int j = 0; j <= numLightsEdge02; j++) {
                        glm::vec3 currentPos01 = parallelogramLight.v0 + (edge01Length / numLightsEdge01) * i * glm::normalize(parallelogramLight.edge01);
                        glm::vec3 currentPos02 = parallelogramLight.v0 + (edge02Length / numLightsEdge02) * j * glm::normalize(parallelogramLight.edge02);
                        glm::vec3 currentPos = currentPos01 + currentPos02 - parallelogramLight.v0;

                        // drawRay({currentPos, glm::vec3(1,1,1), 0.1 }, glm::vec3(1, 0, 0));
                        //areas of sub-parallelograms and the total parallelogram
                        float totalArea = glm::length(glm::cross(parallelogramLight.edge01, parallelogramLight.edge02));

                        float area3 = glm::length(glm::cross(currentPos01 - parallelogramLight.v0, currentPos02 - parallelogramLight.v0));
                        float area0 = glm::length(glm::cross(currentPos01 + parallelogramLight.edge02 - v3, currentPos02 + parallelogramLight.edge01 - v3));
                        float area2 = glm::length(glm::cross(currentPos01 - v1, currentPos02 + parallelogramLight.edge01 - v1));
                        float area1 = glm::length(glm::cross(currentPos02 - v2, currentPos01 + parallelogramLight.edge02 - v2));
                        //color calculated with the weights of each vertex color
                        glm::vec3 currentPosColor = ((area0 * parallelogramLight.color0 / totalArea) + (area1 * parallelogramLight.color1 / totalArea) + (area2 * parallelogramLight.color2 / totalArea) + (area3 * parallelogramLight.color3 / totalArea)) / 400.f;
                        //drawRay({ vertexPos, currentPos - vertexPos, 1.0f }, currentPosColor);

                        const PointLight pointLightCurrent = { currentPos, currentPosColor };
                        color += calculatePhongShading(ray, pointLightCurrent, hitInfo, scene, bvh, recursion);
                    }
                }

            }
        }

        drawRay(ray, color);
        return color;
    } else {
        drawRay(ray, glm::vec3(1.0f, 0.0f, 0.0f)); // Draw a red debug ray if the ray missed.
        return glm::vec3(0.0f); // Set the color of the pixel to black if the ray misses.
    }

    // Lights are stored in a single array (scene.lights) where each item can be either a PointLight, SegmentLight or ParallelogramLight.
    // You can check whether a light at index i is a PointLight using std::holds_alternative:
    // std::holds_alternative<PointLight>(scene.lights[i])
    //
    // If it is indeed a point light, you can "convert" it to the correct type using std::get:
    // PointLight pointLight = std::get<PointLight>(scene.lights[i]);
    //
    //
    // The code to iterate over the lights thus looks like this:
    // for (const auto& light : scene.lights) {
    //     if (std::holds_alternative<PointLight>(light)) {
    //         const PointLight pointLight = std::get<PointLight>(light);
    //         // Perform your calculations for a point light.
    //     } else if (std::holds_alternative<SegmentLight>(light)) {
    //         const SegmentLight segmentLight = std::get<SegmentLight>(light);
    //         // Perform your calculations for a segment light.
    //     } else if (std::holds_alternative<ParallelogramLight>(light)) {
    //         const ParallelogramLight parallelogramLight = std::get<ParallelogramLight>(light);
    //         // Perform your calculations for a parallelogram light.
    //     }
    // }
    //
    // Regarding the soft shadows for **other** light sources **extra** feature:
    // To add a new light source, define your new light struct in scene.h and modify the Scene struct (also in scene.h)
    // by adding your new custom light type to the lights std::variant. For example:
    // std::vector<std::variant<PointLight, SegmentLight, ParallelogramLight, MyCustomLightType>> lights;
    //
    // You can add the light sources programmatically by creating a custom scene (modify the Custom case in the
    // loadScene function in scene.cpp). Custom lights will not be visible in rasterization view.
}

static void setOpenGLMatrices(const Trackball& camera);
static void drawLightsOpenGL(const Scene& scene, const Trackball& camera, int selectedLight);
static void drawSceneOpenGL(const Scene& scene);

glm::vec3 motionBlur(Ray camera, const Scene& scene, const BoundingVolumeHierarchy& bvh) {
    glm::vec3 average{ 0 };
    for(int i = 0; i < 10; i++) {
        camera.origin.x += 0.004f;
        camera.origin.y += 0.004f;
        average += getFinalColor(scene, bvh, camera, 5);
    }
    return average / 10.0f;
}

// This is the main rendering function. You are free to change this function in any way (including the function signature).
static void renderRayTracing(const Scene& scene, const Trackball& camera, const BoundingVolumeHierarchy& bvh, Screen& screen) {
    if(blur) {
    #ifndef NDEBUG
        // Single threaded in debug mode
        for (int y = 0; y < windowResolution.y; y++) {
            for (int x = 0; x != windowResolution.x; x++) {
                // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
                const glm::vec2 normalizedPixelPos {
                        float(x) / windowResolution.x * 2.0f - 1.0f,
                        float(y) / windowResolution.y * 2.0f - 1.0f
                };
                const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                screen.setPixel(x, y, motionBlur(cameraRay,scene, bvh));
            }
        }
    #else
        // Multi-threaded in release mode
    const tbb::blocked_range2d<int, int> windowRange { 0, windowResolution.y, 0, windowResolution.x };
    tbb::parallel_for(windowRange, [&](tbb::blocked_range2d<int, int> localRange) {
        for (int y = std::begin(localRange.rows()); y != std::end(localRange.rows()); y++) {
            for (int x = std::begin(localRange.cols()); x != std::end(localRange.cols()); x++) {
                // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
                const glm::vec2 normalizedPixelPos {
                    float(x) / windowResolution.x * 2.0f - 1.0f,
                    float(y) / windowResolution.y * 2.0f - 1.0f
                };
                const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                screen.setPixel(x, y, motionBlur(cameraRay, scene, bvh));
            }
        }
    });
    #endif
    } else {
        #ifndef NDEBUG
        // Single threaded in debug mode
        for (int y = 0; y < windowResolution.y; y++) {
            for (int x = 0; x != windowResolution.x; x++) {
                // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
                const glm::vec2 normalizedPixelPos {
                        float(x) / windowResolution.x * 2.0f - 1.0f,
                        float(y) / windowResolution.y * 2.0f - 1.0f
                };
                const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, 5));
            }
        }
        #else
        // Multi-threaded in release mode
        const tbb::blocked_range2d<int, int> windowRange { 0, windowResolution.y, 0, windowResolution.x };
        tbb::parallel_for(windowRange, [&](tbb::blocked_range2d<int, int> localRange) {
            for (int y = std::begin(localRange.rows()); y != std::end(localRange.rows()); y++) {
                for (int x = std::begin(localRange.cols()); x != std::end(localRange.cols()); x++) {
                    // NOTE: (-1, -1) at the bottom left of the screen, (+1, +1) at the top right of the screen.
                    const glm::vec2 normalizedPixelPos {
                        float(x) / windowResolution.x * 2.0f - 1.0f,
                        float(y) / windowResolution.y * 2.0f - 1.0f
                    };
                    const Ray cameraRay = camera.generateRay(normalizedPixelPos);
                    screen.setPixel(x, y, getFinalColor(scene, bvh, cameraRay, 5));
                }
            }
        });
    #endif
    }
}

int main(int argc, char** argv)
{
    Trackball::printHelp();
    std::cout << "\n Press the [R] key on your keyboard to create a ray towards the mouse cursor" << std::endl
              << std::endl;

    Window window { "Final Project", windowResolution, OpenGLVersion::GL2 };
    Screen screen { windowResolution };
    Trackball camera { &window, glm::radians(50.0f), 3.0f };
    camera.setCamera(glm::vec3(0.0f, 0.0f, 0.0f), glm::radians(glm::vec3(20.0f, 20.0f, 0.0f)), 3.0f);

    SceneType sceneType { SceneType::SingleTriangle };
    std::optional<Ray> optDebugRay;
    Scene scene = loadScene(sceneType, dataPath);
    BoundingVolumeHierarchy bvh { &scene };

    int bvhDebugLevel = 0;
    bool debugBVH { false };
    bool debugBlur{ false };
    ViewMode viewMode { ViewMode::Rasterization };

    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_R: {
                // Shoot a ray. Produce a ray from camera to the far plane.
                const auto tmp = window.getNormalizedCursorPos();
                optDebugRay = camera.generateRay(tmp * 2.0f - 1.0f);
            } break;
            case GLFW_KEY_ESCAPE: {
                window.close();
            } break;
            };
        }
    });

    int selectedLightIdx = scene.lights.empty() ? -1 : 0;
    while (!window.shouldClose()) {
        window.updateInput();

        // === Setup the UI ===
        ImGui::Begin("Final Project");
        {
            constexpr std::array items { "SingleTriangle", "Cube (segment light)", "Cornell Box (with mirror)", "Cornell Box (parallelogram light and mirror)", "Monkey", "Teapot", "Dragon", /* "AABBs",*/ "Spheres", /*"Mixed",*/ "Custom" };
            if (ImGui::Combo("Scenes", reinterpret_cast<int*>(&sceneType), items.data(), int(items.size()))) {
                optDebugRay.reset();
                scene = loadScene(sceneType, dataPath);
                selectedLightIdx = scene.lights.empty() ? -1 : 0;
                bvh = BoundingVolumeHierarchy(&scene);
                if (optDebugRay) {
                    HitInfo dummy {};
                    bvh.intersect(*optDebugRay, dummy);
                }
            }
        }
        {
            constexpr std::array items { "Rasterization", "Ray Traced" };
            ImGui::Combo("View mode", reinterpret_cast<int*>(&viewMode), items.data(), int(items.size()));
        }
        if (ImGui::Button("Render to file")) {
            // Show a file picker.
            nfdchar_t* pOutPath = nullptr;
            const nfdresult_t result = NFD_SaveDialog("bmp", nullptr, &pOutPath);
            if (result == NFD_OKAY) {
                std::filesystem::path outPath { pOutPath };
                free(pOutPath); // NFD is a C API so we have to manually free the memory it allocated.
                outPath.replace_extension("bmp"); // Make sure that the file extension is *.bmp

                // Perform a new render and measure the time it took to generate the image.
                using clock = std::chrono::high_resolution_clock;
                const auto start = clock::now();
                renderRayTracing(scene, camera, bvh, screen);
                const auto end = clock::now();
                std::cout << "Time to render image: " << std::chrono::duration<float, std::milli>(end - start).count() << " milliseconds" << std::endl;

                // Store the new image.
                screen.writeBitmapToFile(outPath);
            }
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Debugging");
        if (viewMode == ViewMode::Rasterization) {
            ImGui::Checkbox("Draw BVH", &debugBVH);
            if (debugBVH)
                ImGui::SliderInt("BVH Level", &bvhDebugLevel, 0, bvh.numLevels() - 1);

            ImGui::Checkbox("Motion Blur", &debugBlur);
            if (debugBlur) blur = true;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Lights");
        {
            std::vector<std::string> options;
            options.push_back("None");
            for (size_t i = 0; i < scene.lights.size(); i++) {
                options.push_back("Light " + std::to_string(i));
            }
            std::vector<const char*> optionsPointers;
            std::transform(std::begin(options), std::end(options), std::back_inserter(optionsPointers), [](const auto& str) { return str.c_str(); });

            // Offset such that selectedLightIdx=-1 becomes item 0 (None).
            ++selectedLightIdx;
            ImGui::Combo("Selected light", &selectedLightIdx, optionsPointers.data(), static_cast<int>(optionsPointers.size()));
            --selectedLightIdx;

            if (selectedLightIdx >= 0) {
                setOpenGLMatrices(camera);
                std::visit(
                    make_visitor(
                        [&](PointLight& light) {
                            showImGuizmoTranslation(window, camera, light.position); // 3D controls to translate light source.
                            ImGui::DragFloat3("Light position", glm::value_ptr(light.position), 0.01f, -3.0f, 3.0f);
                            ImGui::ColorEdit3("Light color", glm::value_ptr(light.color));
                        },
                        [&](SegmentLight& light) {
                            static int selectedEndpoint = 0;
                            // 3D controls to translate light source.
                            if (selectedEndpoint == 0)
                                showImGuizmoTranslation(window, camera, light.endpoint0);
                            else
                                showImGuizmoTranslation(window, camera, light.endpoint1);

                            const std::array<const char*, 2> endpointOptions { "Endpoint 0", "Endpoint 1" };
                            ImGui::Combo("Selected endpoint", &selectedEndpoint, endpointOptions.data(), (int)endpointOptions.size());
                            ImGui::DragFloat3("Endpoint 0", glm::value_ptr(light.endpoint0), 0.01f, -3.0f, 3.0f);
                            ImGui::DragFloat3("Endpoint 1", glm::value_ptr(light.endpoint1), 0.01f, -3.0f, 3.0f);
                            ImGui::ColorEdit3("Color 0", glm::value_ptr(light.color0));
                            ImGui::ColorEdit3("Color 1", glm::value_ptr(light.color1));
                        },
                        [&](ParallelogramLight& light) {
                            glm::vec3 vertex1 = light.v0 + light.edge01;
                            glm::vec3 vertex2 = light.v0 + light.edge02;

                            static int selectedVertex = 0;
                            // 3D controls to translate light source.
                            if (selectedVertex == 0)
                                showImGuizmoTranslation(window, camera, light.v0);
                            else if (selectedVertex == 1)
                                showImGuizmoTranslation(window, camera, vertex1);
                            else
                                showImGuizmoTranslation(window, camera, vertex2);

                            const std::array<const char*, 3> vertexOptions { "Vertex 0", "Vertex 1", "Vertex 2" };
                            ImGui::Combo("Selected vertex", &selectedVertex, vertexOptions.data(), (int)vertexOptions.size());
                            ImGui::DragFloat3("Vertex 0", glm::value_ptr(light.v0), 0.01f, -3.0f, 3.0f);
                            ImGui::DragFloat3("Vertex 1", glm::value_ptr(vertex1), 0.01f, -3.0f, 3.0f);
                            light.edge01 = vertex1 - light.v0;
                            ImGui::DragFloat3("Vertex 2", glm::value_ptr(vertex2), 0.01f, -3.0f, 3.0f);
                            light.edge02 = vertex2 - light.v0;

                            ImGui::ColorEdit3("Color 0", glm::value_ptr(light.color0));
                            ImGui::ColorEdit3("Color 1", glm::value_ptr(light.color1));
                            ImGui::ColorEdit3("Color 2", glm::value_ptr(light.color2));
                            ImGui::ColorEdit3("Color 3", glm::value_ptr(light.color3));
                        },
                        [](auto) { /* any other type of light */ }),
                    scene.lights[selectedLightIdx]);
            }
        }

        if (ImGui::Button("Add point light")) {
            selectedLightIdx = int(scene.lights.size());
            scene.lights.push_back(PointLight { .position = glm::vec3(0.0f), .color = glm::vec3(1.0f) });
        }
        if (ImGui::Button("Add segment light")) {
            selectedLightIdx = int(scene.lights.size());
            scene.lights.push_back(SegmentLight { .endpoint0 = glm::vec3(0.0f), .endpoint1 = glm::vec3(1.0f), .color0 = glm::vec3(1, 0, 0), .color1 = glm::vec3(0, 0, 1) });
        }
        if (ImGui::Button("Add parallelogram light")) {
            selectedLightIdx = int(scene.lights.size());
            scene.lights.push_back(ParallelogramLight {
                .v0 = glm::vec3(0.0f),
                .edge01 = glm::vec3(1, 0, 0),
                .edge02 = glm::vec3(0, 1, 0),
                .color0 = glm::vec3(1, 0, 0), // red
                .color1 = glm::vec3(0, 1, 0), // green
                .color2 = glm::vec3(0, 0, 1), // blue
                .color3 = glm::vec3(1, 1, 1) // white
            });
        }
        if (selectedLightIdx >= 0 && ImGui::Button("Remove selected light")) {
            scene.lights.erase(std::begin(scene.lights) + selectedLightIdx);
            selectedLightIdx = -1;
        }

        // Clear screen.
        glViewport(0, 0, window.getFrameBufferSize().x, window.getFrameBufferSize().y);
        glClearDepth(1.0f);
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        setOpenGLMatrices(camera);

        // Draw either using OpenGL (rasterization) or the ray tracing function.
        switch (viewMode) {
        case ViewMode::Rasterization: {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            drawSceneOpenGL(scene);
            if (optDebugRay) {
                // Call getFinalColor for the debug ray. Ignore the result but tell the function that it should
                // draw the rays instead.
                enableDrawRay = true;
                (void)getFinalColor(scene, bvh, *optDebugRay, 5);
                enableDrawRay = false;
            }
            glPopAttrib();
        } break;
        case ViewMode::RayTracing: {
            screen.clear(glm::vec3(0.0f));
            renderRayTracing(scene, camera, bvh, screen);
            screen.setPixel(0, 0, glm::vec3(1.0f));
            screen.draw(); // Takes the image generated using ray tracing and outputs it to the screen using OpenGL.
        } break;
        default:
            break;
        };

        drawLightsOpenGL(scene, camera, selectedLightIdx);

        if (debugBVH) {
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            setOpenGLMatrices(camera);
            glDisable(GL_LIGHTING);
            glEnable(GL_DEPTH_TEST);

            // Enable alpha blending. More info at:
            // https://learnopengl.com/Advanced-OpenGL/Blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            enableDrawRay = true;
            bvh.debugDraw(bvhDebugLevel);
            enableDrawRay = false;
            glPopAttrib();
        }

        ImGui::End();
        window.swapBuffers();
    }

    return 0;
}

static void setOpenGLMatrices(const Trackball& camera)
{
    // Load view matrix.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    const glm::mat4 viewMatrix = camera.viewMatrix();
    glMultMatrixf(glm::value_ptr(viewMatrix));

    // Load projection matrix.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    const glm::mat4 projectionMatrix = camera.projectionMatrix();
    glMultMatrixf(glm::value_ptr(projectionMatrix));
}

static void drawLightsOpenGL(const Scene& scene, const Trackball& camera, int selectedLight)
{
    // Normals will be normalized in the graphics pipeline.
    glEnable(GL_NORMALIZE);
    // Activate rendering modes.
    glEnable(GL_DEPTH_TEST);
    // Draw front and back facing triangles filled.
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);
    // Interpolate vertex colors over the triangles.
    glShadeModel(GL_SMOOTH);

    glDisable(GL_LIGHTING);
    // Draw all non-selected lights.
    for (size_t i = 0; i < scene.lights.size(); i++) {
        std::visit(
            make_visitor(
                [](const PointLight& light) { drawSphere(light.position, 0.01f, light.color); },
                [](const SegmentLight& light) {
                    glPushAttrib(GL_ALL_ATTRIB_BITS);
                    glBegin(GL_LINES);
                    glColor3fv(glm::value_ptr(light.color0));
                    glVertex3fv(glm::value_ptr(light.endpoint0));
                    glColor3fv(glm::value_ptr(light.color1));
                    glVertex3fv(glm::value_ptr(light.endpoint1));
                    glEnd();
                    glPopAttrib();
                    drawSphere(light.endpoint0, 0.01f, light.color0);
                    drawSphere(light.endpoint1, 0.01f, light.color1);
                },
                [](const ParallelogramLight& light) {
                    glPushAttrib(GL_ALL_ATTRIB_BITS);
                    glBegin(GL_QUADS);
                    glColor3fv(glm::value_ptr(light.color0));
                    glVertex3fv(glm::value_ptr(light.v0));
                    glColor3fv(glm::value_ptr(light.color1));
                    glVertex3fv(glm::value_ptr(light.v0 + light.edge01));
                    glColor3fv(glm::value_ptr(light.color3));
                    glVertex3fv(glm::value_ptr(light.v0 + light.edge01 + light.edge02));
                    glColor3fv(glm::value_ptr(light.color2));
                    glVertex3fv(glm::value_ptr(light.v0 + light.edge02));
                    glEnd();
                    glPopAttrib();
                },
                [](auto) { /* any other type of light */ }),
            scene.lights[i]);
    }

    // Draw a colored sphere at the location at which the trackball is looking/rotating around.
    glDisable(GL_LIGHTING);
    drawSphere(camera.lookAt(), 0.01f, glm::vec3(0.2f, 0.2f, 1.0f));
}

void drawSceneOpenGL(const Scene& scene)
{
    // Activate the light in the legacy OpenGL mode.
    glEnable(GL_LIGHTING);

    // Tell OpenGL where the lights are (so it nows how to shade surfaces in the scene).
    // This is only used in the rasterization view. OpenGL only supports point lights so
    // we replace segment/parallelogram lights by point lights.
    int i = 0;
    const auto enableLight = [&](const glm::vec3& position, const glm::vec3 color) {
        glEnable(GL_LIGHT0 + i);
        const glm::vec4 position4 { position, 1 };
        glLightfv(GL_LIGHT0 + i, GL_POSITION, glm::value_ptr(position4));
        const glm::vec4 color4 { glm::clamp(color, 0.0f, 1.0f), 1.0f };
        const glm::vec4 zero4 { 0.0f, 0.0f, 0.0f, 1.0f };
        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, glm::value_ptr(zero4));
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, glm::value_ptr(color4));
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, glm::value_ptr(zero4));
        // NOTE: quadratic attenuation doesn't work like you think it would in legacy OpenGL.
        // The distance is not in world space but in NDC space!
        glLightf(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.0f);
        glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, 0.0f);
        i++;
    };
    for (const auto& light : scene.lights) {
        std::visit(
            make_visitor(
                [&](const PointLight& light) {
                    enableLight(light.position, light.color);
                },
                [&](const SegmentLight& light) {
                    // Approximate with two point lights: one at each endpoint.
                    enableLight(light.endpoint0, 0.5f * light.color0);
                    enableLight(light.endpoint1, 0.5f * light.color1);
                },
                [&](const ParallelogramLight& light) {
                    enableLight(light.v0, 0.25f * light.color0);
                    enableLight(light.v0 + light.edge01, 0.25f * light.color1);
                    enableLight(light.v0 + light.edge02, 0.25f * light.color2);
                    enableLight(light.v0 + light.edge01 + light.edge02, 0.25f * light.color3);
                },
                [](auto) { /* any other type of light */ }),
            light);
    }

    // Draw the scene and the ray (if any).
    drawScene(scene);
}
