#pragma once
#include "ray_tracing.h"
#include "scene.h"
#include <array>
#include <span>
#include <glm/mat3x3.hpp>
class BoundingVolumeHierarchy {
public:
    BoundingVolumeHierarchy(Scene* pScene);

    struct Node {
        bool isLeaf;
        std::vector<int> indices;
        glm::vec3 lower; //Lower coordinate of the AABB
        glm::vec3 upper; //Upper coordinate of the AABB
    };
    std::vector<Node> nodes;
    int maxDepth; // Depth of the tree. The root starts at 0

    std::vector<glm::mat3> allTriangles; //A triangle has 3 vertices and each vertex has xyz coordinates -> a 3x3 matrix
    std::vector<int> meshIndices; //The i'th position corresponds to triangle i and the value at the i'th position is the mesh index.
    std::vector<std::vector<Vertex>> vertexIndices; //The i'th position corresponds to triangle i with the 3 vertices

    // Implement these two functions for the Visual Debug.
    // The first function should return how many levels there are in the tree that you have constructed.
    // The second function should draw the bounding boxes of the nodes at the selected level.
    int numLevels() const;
    void debugDraw(int level);

    // Return true if something is hit, returns false otherwise.
    // Only find hits if they are closer than t stored in the ray and the intersection
    // is on the correct side of the origin (the new t >= 0).
    bool intersect(Ray& ray, HitInfo& hitInfo) const;



private:
    Scene* m_pScene;
};
