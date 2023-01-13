#pragma once
#include "scene.h"
#include <framework/mesh.h>
#include <framework/ray.h>
#include <utility> // std::forward
#include <glm/mat3x3.hpp>

enum class DrawMode {
    Filled,
    Wireframe
};


// Add your own custom visual debug draw functions here then implement it in draw.cpp.
// You are free to modify the example one however you like.
void drawExampleOfCustomVisualDebug();

extern bool enableDrawRay;
void drawRay(const Ray& ray, const glm::vec3& color = glm::vec3(1.0f));

void drawAABB(const AxisAlignedBox& box, DrawMode drawMode = DrawMode::Filled, const glm::vec3& color = glm::vec3(1.0f), float transparency = 1.0f);

void drawMesh(const Mesh& mesh);
void drawATriangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2);
void drawTriangles(std::vector<int> indices, glm::vec3 color, std::vector<glm::mat3> allTriangles);
void drawSphere(const Sphere& sphere);
void drawSphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f));
void drawScene(const Scene& scene);

