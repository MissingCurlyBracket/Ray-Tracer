#include "ray_tracing.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/geometric.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/vector_relational.hpp>
DISABLE_WARNINGS_POP()
#include <cmath>
#include <iostream>
#include <limits>

bool pointInTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& n, const glm::vec3& p)
{
    float alpha = glm::dot(n, glm::cross(v2 - v1, p - v1)) / pow(glm::length(n), 2.0f);
    float beta = glm::dot(n, glm::cross(v0 - v2, p - v2)) / pow(glm::length(n), 2.0f);

    return !(alpha < 0 || beta < 0 || alpha + beta > 1);
}

bool intersectRayWithPlane(const Plane& plane, Ray& ray)
{
    float t = (plane.D - glm::dot(ray.origin, plane.normal)) / glm::dot(ray.direction, plane.normal);
    if(t < ray.t && t > 0 && glm::dot(plane.normal, ray.direction) != 0) {
        ray.t = t;
        return true;
    }
    else return false;
}

Plane trianglePlane(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
    Plane plane;
    plane.normal = glm::normalize(glm::cross(v0 - v2, v1 - v2));
    plane.D = glm::dot(plane.normal, v0);
    return plane;
}

/// Input: the three vertices of the triangle
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithTriangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, Ray& ray, HitInfo& hitInfo)
{
    Plane plane = trianglePlane(v0, v1, v2);
    float originalT = ray.t;
    if(intersectRayWithPlane(plane, ray)) {
        if(pointInTriangle(v0, v1, v2, glm::cross(v0 - v2, v1 - v2), ray.origin + ray.direction * ray.t)) {
            hitInfo.normal = plane.normal;
            return true;
        } else {
            ray.t = originalT;
            return false;
        }
    } return false;
}

/// Input: a sphere with the following attributes: sphere.radius, sphere.center
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const Sphere& sphere, Ray& ray, HitInfo& hitInfo)
{
    glm::vec3 newOrigin = ray.origin - sphere.center;
    float A = glm::pow(ray.direction.x, 2) + glm::pow(ray.direction.y, 2) + glm::pow(ray.direction.z, 2);
    float B = 2.0f * ((ray.direction.x * newOrigin.x) + (ray.direction.y * newOrigin.y) + (ray.direction.z * newOrigin.z));
    float C = glm::pow(newOrigin.x, 2) + glm::pow(newOrigin.y, 2) + glm::pow(newOrigin.z, 2) - glm::pow(sphere.radius, 2);

    float discriminant = glm::pow(B, 2) - 4 * A * C;

    if (discriminant < 0) //ray doesn't intersect the sphere
        return false;
    else if (discriminant == 0) { //one unique solution
        if ((-B) / (2 * A) > 0 && (-B) / (2 * A) < ray.t) { //the intersection is in front of the camera and nearer than the previous intersection
            ray.t = (-B) / (2 * A);
            hitInfo.normal = glm::normalize((ray.origin + ray.t * ray.direction) - sphere.center);
            return true;
        }
        else return false;
    }
    else { //two unique solutions
        float first = (-B + glm::sqrt(discriminant)) / (2 * A);
        float second = (-B - glm::sqrt(discriminant)) / (2 * A);

        if (first < 0 && second > 0 && second < ray.t) { //if the origin is in the sphere, choose the intersection in front
            ray.t = second;
            hitInfo.normal = glm::normalize((ray.origin + ray.t * ray.direction) - sphere.center);
            hitInfo.material = sphere.material;
            return true;
        }
        else if (second < 0 && first > 0 && first < ray.t) { //if the origin is in the sphere, choose the intersection in front
            ray.t = first;
            hitInfo.normal = glm::normalize((ray.origin + ray.t * ray.direction) - sphere.center);
            hitInfo.material = sphere.material;
            return true;
        }
        else if (first > 0 && second > 0 && glm::min(first, second) < ray.t) { //both of the intersections are in front of the camera
            ray.t = glm::min(first, second); //choose the smallest one if it's nearer than the previous intersection.
            hitInfo.normal = glm::normalize((ray.origin + ray.t * ray.direction) - sphere.center);
            hitInfo.material = sphere.material;
            return true;
        }
        else {
            return false; //both intersections are behind the camera
        }
    }
}

/// Input: an axis-aligned bounding box with the following parameters: minimum coordinates box.lower and maximum coordinates box.upper
/// Output: if intersects then modify the hit parameter ray.t and return true, otherwise return false
bool intersectRayWithShape(const AxisAlignedBox& box, Ray& ray)
{
    glm::vec3 min = box.lower;
    glm::vec3 max = box.upper;
    glm::vec3 a = glm::vec3(max.x, min.y, min.z);
    glm::vec3 b = glm::vec3(min.x, max.y, min.z);
    glm::vec3 c = glm::vec3(min.x, min.y, max.z);
    glm::vec3 d = glm::vec3(max.x, max.y, min.z);
    glm::vec3 e = glm::vec3(max.x, min.y, max.z);
    glm::vec3 f = glm::vec3(min.x, max.y, max.z);

    HitInfo hitInfo;
    bool b1 = intersectRayWithTriangle(min, a, b, ray, hitInfo);
    bool b2 = intersectRayWithTriangle(d, b, a, ray, hitInfo);
    bool b3 = intersectRayWithTriangle(min, a, c, ray, hitInfo);
    bool b4 = intersectRayWithTriangle(e, c, a, ray, hitInfo);
    bool b5 = intersectRayWithTriangle(min, b, c, ray, hitInfo);
    bool b6 = intersectRayWithTriangle(f, b, c, ray, hitInfo);
    bool b7 = intersectRayWithTriangle(a, d, e, ray, hitInfo);
    bool b8 = intersectRayWithTriangle(max, d, e, ray, hitInfo);
    bool b9 = intersectRayWithTriangle(b, f, d, ray, hitInfo);
    bool b10 = intersectRayWithTriangle(max, f, d, ray, hitInfo);
    bool b11 = intersectRayWithTriangle(max, f, e, ray, hitInfo);
    bool b12 = intersectRayWithTriangle(c, f, e, ray, hitInfo);

    return (b1 || b2 || b3 || b4 || b5 || b6 || b7 || b8 || b9 || b10 || b11 || b12);
}
