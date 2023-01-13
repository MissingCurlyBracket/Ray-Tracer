#pragma once
#include "scene.h"
#include "glm/mat3x3.hpp"

struct HitInfo {
    glm::vec3 normal;
    Material material;
    glm::mat3 finalTriangleVertices;
};

bool intersectRayWithPlane(const Plane& plane, Ray& ray);

// Returns true if the point p is inside the triangle spanned by v0, v1, v2 with normal n.
bool pointInTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& n, const glm::vec3& p);

Plane trianglePlane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2);

bool intersectRayWithTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, Ray& ray, HitInfo& hitInfo);
bool intersectRayWithShape(const Sphere& sphere, Ray& ray, HitInfo& hitInfo);
bool intersectRayWithShape(const AxisAlignedBox& box, Ray& ray);
