#pragma once
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <framework/mesh.h>
#include <framework/ray.h>
#include <optional>
#include <vector>
#include <variant>

enum SceneType {
    SingleTriangle,
    Cube,
    CornellBox,
    CornellBoxParallelogramLight,
    Monkey,
    Teapot,
    Dragon,
    //AABBs,
    Spheres,
    //Mixed,
    Custom
};

struct Plane {
    float D = 0.0f;
    glm::vec3 normal { 0.0f, 1.0f, 0.0f };
};

struct AxisAlignedBox {
    glm::vec3 lower { 0.0f };
    glm::vec3 upper { 1.0f };
};

struct Sphere {
    glm::vec3 center { 0.0f };
    float radius = 1.0f;
    Material material;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
};

struct SegmentLight {
    glm::vec3 endpoint0, endpoint1; // Positions of endpoints
    glm::vec3 color0, color1; // Color of endpoints
};

struct ParallelogramLight {
    // A parallelogram light (see figure 3.14 of chapter 13.4.2 of Fundamentals of CG 4th Edition)
    glm::vec3 v0; // v0
    glm::vec3 edge01, edge02; // edges from v0 to v1, and from v0 to v2
    glm::vec3 color0, color1, color2, color3;


};

struct Scene {
    std::vector<Mesh> meshes;
    std::vector<Sphere> spheres;
    //std::vector<AxisAlignedBox> boxes;

    std::vector<std::variant<PointLight, SegmentLight, ParallelogramLight>> lights;
};

// Load a prebuilt scene.
Scene loadScene(SceneType type, const std::filesystem::path& dataDir);
