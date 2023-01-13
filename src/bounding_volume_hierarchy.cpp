#include "bounding_volume_hierarchy.h"
#include "draw.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <imgui.h>
#include <nfd.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
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

float centroidOfTriangle(glm::mat3 triangle, int axis) {
    switch (axis) {
        case 0:
            return (triangle[0].x + triangle[1].x + triangle[2].x) / 3;
        case 1:
            return (triangle[0].y + triangle[1].y + triangle[2].y) / 3;
        case 2:
            return (triangle[0].z + triangle[1].z + triangle[2].z) / 3;
        default:
            return -1.0f;
    }
}

bool compareTrianglesX(glm::mat3 triangle1, glm::mat3 triangle2) {
    return centroidOfTriangle(triangle1, 0) < centroidOfTriangle(triangle2, 0);
}

bool compareTrianglesY(glm::mat3 triangle1, glm::mat3 triangle2) {
    return centroidOfTriangle(triangle1, 1) < centroidOfTriangle(triangle2, 1);
}

bool compareTrianglesZ(glm::mat3 triangle1, glm::mat3 triangle2) {
    return centroidOfTriangle(triangle1, 2) < centroidOfTriangle(triangle2, 2);
}

std::pair<glm::vec3, glm::vec3> calculateLowerUpper(std::vector<std::pair<glm::mat3, int>> trianglesAndIndices) {
    float xmin, ymin, zmin;
    xmin = ymin = zmin = std::numeric_limits<float>::infinity();
    float xmax, ymax, zmax;
    xmax = ymax = zmax = -1 * std::numeric_limits<float>::infinity();
    for(std::pair<glm::mat3, int> triangleAndIndex : trianglesAndIndices) {
        glm::vec3 v0 = triangleAndIndex.first[0];
        glm::vec3 v1 = triangleAndIndex.first[1];
        glm::vec3 v2 = triangleAndIndex.first[2];

        if(v0.x < xmin) xmin = v0.x;
        if(v0.y < ymin) ymin = v0.y;
        if(v0.z < zmin) zmin = v0.z;
        if(v0.x > xmax) xmax = v0.x;
        if(v0.y > ymax) ymax = v0.y;
        if(v0.z > zmax) zmax = v0.z;

        if(v1.x < xmin) xmin = v1.x;
        if(v1.y < ymin) ymin = v1.y;
        if(v1.z < zmin) zmin = v1.z;
        if(v1.x > xmax) xmax = v1.x;
        if(v1.y > ymax) ymax = v1.y;
        if(v1.z > zmax) zmax = v1.z;

        if(v2.x < xmin) xmin = v2.x;
        if(v2.y < ymin) ymin = v2.y;
        if(v2.z < zmin) zmin = v2.z;
        if(v2.x > xmax) xmax = v2.x;
        if(v2.y > ymax) ymax = v2.y;
        if(v2.z > zmax) zmax = v2.z;
    }
    glm::vec3 lower = glm::vec3(xmin, ymin, zmin);
    glm::vec3 upper = glm::vec3(xmax, ymax, zmax);

    return std::pair(lower, upper);
}

std::pair<std::vector<glm::vec3>, std::vector<std::vector<std::pair<glm::mat3, int>>>> doSplitting(std::vector<std::pair<glm::mat3, int>>& trianglesAndIndices, int axis) {
    std::vector<glm::mat3> triangles;
    for(std::pair<glm::mat3, int> tri : trianglesAndIndices) {
        triangles.push_back(tri.first);
    }

    if(axis % 3 == 0) std::sort(triangles.begin(), triangles.end(), compareTrianglesX);
    else if(axis % 3 == 1) std::sort(triangles.begin(), triangles.end(), compareTrianglesY);
    else std::sort(triangles.begin(), triangles.end(), compareTrianglesZ);
    glm::mat3 medianTriangle = triangles[triangles.size() / 2];

    glm::vec3 splittingVertex;
    std::vector<std::pair<glm::mat3, int>> firstAABBTrianglesAndIndices;
    if(axis % 3 == 0) {
        if(medianTriangle[0].x > medianTriangle[1].x && medianTriangle[0].x > medianTriangle[2].x) {
            splittingVertex = medianTriangle[0];
        } else if(medianTriangle[1].x > medianTriangle[0].x && medianTriangle[1].x > medianTriangle[2].x) {
            splittingVertex = medianTriangle[1];
        } else splittingVertex = medianTriangle[2];

        for(std::pair<glm::mat3, int> triangleAndIndex : trianglesAndIndices) {
            glm::vec3 v0 = triangleAndIndex.first[0];
            glm::vec3 v1 = triangleAndIndex.first[1];
            glm::vec3 v2 = triangleAndIndex.first[2];

            if(v0.x <= splittingVertex.x && v1.x <= splittingVertex.x && v2.x <= splittingVertex.x) firstAABBTrianglesAndIndices.push_back(triangleAndIndex);
        }
    }
    else if(axis % 3 == 1) {
        if(medianTriangle[0].y < medianTriangle[1].y && medianTriangle[0].y < medianTriangle[2].y) {
            splittingVertex = medianTriangle[0];
        } else if(medianTriangle[1].y < medianTriangle[0].y && medianTriangle[1].y < medianTriangle[2].y) {
            splittingVertex = medianTriangle[1];
        } else splittingVertex = medianTriangle[2];

        for(std::pair<glm::mat3, int> triangleAndIndex : trianglesAndIndices) {
            glm::vec3 v0 = triangleAndIndex.first[0];
            glm::vec3 v1 = triangleAndIndex.first[1];
            glm::vec3 v2 = triangleAndIndex.first[2];

            if(v0.y >= splittingVertex.y && v1.y >= splittingVertex.y && v2.y >= splittingVertex.y) firstAABBTrianglesAndIndices.push_back(triangleAndIndex);
        }
    } else {
        if(medianTriangle[0].z < medianTriangle[1].z && medianTriangle[0].z < medianTriangle[2].z) {
            splittingVertex = medianTriangle[0];
        } else if(medianTriangle[1].z < medianTriangle[0].z && medianTriangle[1].z < medianTriangle[2].z) {
            splittingVertex = medianTriangle[1];
        } else splittingVertex = medianTriangle[2];

        for(std::pair<glm::mat3, int> triangleAndIndex : trianglesAndIndices) {
            glm::vec3 v0 = triangleAndIndex.first[0];
            glm::vec3 v1 = triangleAndIndex.first[1];
            glm::vec3 v2 = triangleAndIndex.first[2];

            if(v0.z >= splittingVertex.z && v1.z >= splittingVertex.z && v2.z >= splittingVertex.z) firstAABBTrianglesAndIndices.push_back(triangleAndIndex);
        }
    }

    //Calculating triangles of the second AABB. This is done by subtracting the triangles in the first AABB from the total triangles.
    std::vector<int> indiceListCurrentAABB; //Triangle indices of the current AABB (before splitting)
    std::vector<int> indiceListFirstAABB; //Triangle indices of the first child AABB
    for(std::pair<glm::mat3, int> triangleAndIndex : trianglesAndIndices) {
        indiceListCurrentAABB.push_back(triangleAndIndex.second);
    }
    for(std::pair<glm::mat3, int> triangleAndIndex : firstAABBTrianglesAndIndices) {
        indiceListFirstAABB.push_back(triangleAndIndex.second);
    }

    std::vector<int> indiceListSecondAABB;
    std::set_difference(indiceListCurrentAABB.begin(), indiceListCurrentAABB.end(), indiceListFirstAABB.begin(), indiceListFirstAABB.end(), std::inserter(indiceListSecondAABB, indiceListSecondAABB.begin()));

    std::vector<std::pair<glm::mat3, int>> secondAABBTrianglesAndIndices;
    for(std::pair<glm::mat3, int> triangleAndIndex : trianglesAndIndices) {
        for(int index : indiceListSecondAABB) {
            if(triangleAndIndex.second == index) {
                secondAABBTrianglesAndIndices.push_back(triangleAndIndex);
                break;
            }
        }
    }

    //Calculating the bounding boxes for the AABB
    std::pair<glm::vec3, glm::vec3> leftAABB = calculateLowerUpper(firstAABBTrianglesAndIndices);
    std::pair<glm::vec3, glm::vec3> rightAABB = calculateLowerUpper(secondAABBTrianglesAndIndices);

    return std::pair(std::vector{leftAABB.first, leftAABB.second, rightAABB.first, rightAABB.second}, std::vector{firstAABBTrianglesAndIndices, secondAABBTrianglesAndIndices});
}

void fillNodeVector(int maxDepth, int currentLevel, glm::vec3 lower, glm::vec3 upper, std::vector<BoundingVolumeHierarchy::Node>& nodes, std::vector<std::pair<glm::mat3, int>> trianglesAndIndices, int axis) {
    BoundingVolumeHierarchy::Node node;
    node.lower = lower;
    node.upper = upper;

    if(currentLevel == maxDepth || trianglesAndIndices.size() <= 1) { //Base case -> leaf nodes.
        node.isLeaf = true;

        std::vector<int> theList;
        for(std::pair<glm::mat3, int> pair : trianglesAndIndices) {
            theList.push_back(pair.second);
        }
        node.indices = theList;

        nodes.push_back(node);
        return;
    }

    node.isLeaf = false;

    std::vector<glm::mat3> triangles;
    for(std::pair<glm::mat3, int> tri : trianglesAndIndices) {
        triangles.push_back(tri.first);
    }

    //Splitting criterion is in the assignment description. Axis is an integer -> if 0 we split along the x-axis, if 1 along the y-axis and if 2 along the z-axis.
    std::pair<std::vector<glm::vec3>, std::vector<std::vector<std::pair<glm::mat3, int>>>> data;
    if(axis % 3 == 0) data = doSplitting(trianglesAndIndices, axis); //Split on x-axis
    else if(axis % 3 == 1)data = doSplitting(trianglesAndIndices, axis); //Split on y-axis
    else data = doSplitting(trianglesAndIndices, axis); //Split on z-axis

    glm::vec3 aabb1Lower = data.first[0];
    glm::vec3 aabb1Upper = data.first[1];
    glm::vec3 aabb2Lower = data.first[2];
    glm::vec3 aabb2Upper = data.first[3];
    std::vector<std::pair<glm::mat3, int>> firstAABBTriangles = data.second[0];
    std::vector<std::pair<glm::mat3, int>> secondAABBTriangles = data.second[1];

    //Recursively make left child nodes
    fillNodeVector(maxDepth, currentLevel+1, aabb1Lower, aabb1Upper, nodes, firstAABBTriangles, axis+1);
    int childNode1 = nodes.size() - 1;

    //Recursively make right child nodes
    fillNodeVector(maxDepth, currentLevel+1, aabb2Lower, aabb2Upper, nodes, secondAABBTriangles, axis+1);
    int childNode2 = nodes.size() - 1;

    node.indices = std::vector<int> {childNode1, childNode2}; //These are the child node indices of an inner node.
    nodes.push_back(node);
}

BoundingVolumeHierarchy::BoundingVolumeHierarchy(Scene* pScene): m_pScene(pScene) {
    maxDepth = numLevels();

    //Define the lower and upper coordinates for the entire scene
    float xmin, ymin, zmin;
    xmin = ymin = zmin = std::numeric_limits<float>::infinity();
    float xmax, ymax, zmax;
    xmax = ymax = zmax = -1 * std::numeric_limits<float>::infinity();
    for(const auto& mesh : m_pScene->meshes) {
        for(const auto& vertex : mesh.vertices) {
            if(vertex.position.x < xmin) xmin = vertex.position.x;
            if(vertex.position.y < ymin) ymin = vertex.position.y;
            if(vertex.position.z < zmin) zmin = vertex.position.z;
            if(vertex.position.x > xmax) xmax = vertex.position.x;
            if(vertex.position.y > ymax) ymax = vertex.position.y;
            if(vertex.position.z > zmax) zmax = vertex.position.z;
        }
    }
    glm::vec3 lower = glm::vec3(xmin, ymin, zmin);
    glm::vec3 upper = glm::vec3(xmax, ymax, zmax);

    //Make a list of all the triangles with its positions
    //Every row is the xyz coordinate of 1 vertex and there are 3 vertices per triangle -> 3x3 matrix. The int corresponds to the index that it will have in the allTriangles array
    std::vector<std::pair<glm::mat3, int>> triangles;
    int triangleCounter = 0;
    int meshCounter = 0;
    for(const auto& mesh : m_pScene->meshes) {
        for(const auto& triangle : mesh.triangles) {
            Vertex v0 = mesh.vertices[triangle.x];
            Vertex v1 = mesh.vertices[triangle.y];
            Vertex v2 = mesh.vertices[triangle.z];

            glm::mat3 triangleWithPositions = glm::mat3(v0.position, v1.position, v2.position);
            triangles.push_back(std::pair(triangleWithPositions, triangleCounter++));
            allTriangles.push_back(triangleWithPositions);
            meshIndices.push_back(meshCounter);
            vertexIndices.push_back(std::vector{v0, v1, v2});
        }
        meshCounter++;
    }

    fillNodeVector(maxDepth-1, 0, lower, upper, nodes, triangles, 0);
}

// Return the depth of the tree that you constructed. This is used to tell the
// slider in the UI how many steps it should display.
int BoundingVolumeHierarchy::numLevels() const {
    return 4;
}

std::vector<BoundingVolumeHierarchy::Node> getLeafNodes(std::vector<BoundingVolumeHierarchy::Node>& nodes, BoundingVolumeHierarchy::Node root) {
    if(root.isLeaf) return std::vector<BoundingVolumeHierarchy::Node>{root};

    std::vector<BoundingVolumeHierarchy::Node> v1;
    std::vector<BoundingVolumeHierarchy::Node> v2;
    if(root.indices.size() == 2) {
        v1 = getLeafNodes(nodes, nodes[root.indices[0]]);
        v2 = getLeafNodes(nodes, nodes[root.indices[1]]);
    }

    //Just concatenates two vectors. The nodes found on the left subtree and the nodes found on the right subtree.
    std::vector<BoundingVolumeHierarchy::Node> nodesAtGivenLevel;
    if(v1.size() != 0) nodesAtGivenLevel.insert(nodesAtGivenLevel.end(), v1.begin(), v1.end());
    if(v2.size() != 0) nodesAtGivenLevel.insert(nodesAtGivenLevel.end(), v2.begin(), v2.end());
    return nodesAtGivenLevel;
}

std::vector<BoundingVolumeHierarchy::Node> getNodesAtLevel(std::vector<BoundingVolumeHierarchy::Node>& nodes, BoundingVolumeHierarchy::Node root, int currentLevel, int targetLevel) {
    if(currentLevel == targetLevel) return std::vector<BoundingVolumeHierarchy::Node>{root};

    std::vector<BoundingVolumeHierarchy::Node> v1;
    std::vector<BoundingVolumeHierarchy::Node> v2;
    if(root.indices.size() == 2) {
        v1 = getNodesAtLevel(nodes, nodes[root.indices[0]], currentLevel+1, targetLevel);
        v2 = getNodesAtLevel(nodes, nodes[root.indices[1]], currentLevel+1, targetLevel);
    }

    //Just concatenates two vectors. The nodes found on the left subtree and the nodes found on the right subtree.
    std::vector<BoundingVolumeHierarchy::Node> nodesAtGivenLevel;
    if(v1.size() != 0) nodesAtGivenLevel.insert(nodesAtGivenLevel.end(), v1.begin(), v1.end());
    if(v2.size() != 0) nodesAtGivenLevel.insert(nodesAtGivenLevel.end(), v2.begin(), v2.end());
    return nodesAtGivenLevel;
}

// Use this function to visualize your BVH. This can be useful for debugging. Use the functions in
// draw.h to draw the various shapes. We have extended the AABB draw functions to support wireframe
// mode, arbitrary colors and transparency.
void BoundingVolumeHierarchy::debugDraw(int level) {
    // Draw the AABB as a transparent green box.
    //AxisAlignedBox aabb{ glm::vec3(-0.05f), glm::vec3(0.05f, 1.05f, 1.05f) };
    //drawShape(aabb, DrawMode::Filled, glm::vec3(0.0f, 1.0f, 0.0f), 0.2f);

    bool left = true;
    //Get the nodes at a specific level of the BVH tree and draw the AABB.
    for(BoundingVolumeHierarchy::Node node : getNodesAtLevel(nodes, nodes.back(), 0, level)) {
        if (left) {
            drawAABB(AxisAlignedBox{ node.lower, node.upper }, DrawMode::Wireframe, glm::vec3{0,1,0});
            left = false;
        }
        else {
            drawAABB(AxisAlignedBox{ node.lower, node.upper }, DrawMode::Wireframe, glm::vec3{1,0,0});
            left = true;
        }
    }

    //Coloring all the leaf nodes of the BVH
//    glm::vec3 color = glm::vec3(0, 0, 0);
//    std::vector<Node> leafNodes = getLeafNodes(nodes, nodes.back());
//    float range = 255.0f / leafNodes.size();
//    for(BoundingVolumeHierarchy::Node node : leafNodes) {
//        drawTriangles(node.indices, color, allTriangles);
//        color.y += (range / 255.0f);
//    }

    // Draw the AABB as a (white) wireframe box./
    //AxisAlignedBox aabb {nodes[0].lower, nodes[0].upper };
    //drawAABB(aabb, DrawMode::Wireframe);
    //drawAABB(aabb, DrawMode::Filled, glm::vec3(0.05f, 1.0f, 0.05f), 0.1f);
}

bool intersect(const std::vector<BoundingVolumeHierarchy::Node>& nodes, BoundingVolumeHierarchy::Node root, Ray& ray, HitInfo& hitInfo, const std::vector<int>& meshIndices, std::vector<Mesh>& meshes, const std::vector<std::vector<Vertex>>& vertexIndices) {
    if(root.isLeaf) {
        //drawTriangles(root.indices, glm::vec3(1, 0 , 0), triangles); //Uncomment this to draw all the triangles of the leaf node of the intersected triangle
        bool hit = false;
        drawAABB(AxisAlignedBox{root.lower, root.upper}, DrawMode::Wireframe, glm::vec3(0, 0, 1)); //Draws the intersected AABBs. For some reason the color doesn't work...
        for(int index : root.indices) {
            Vertex v0 = vertexIndices[index][0];
            Vertex v1 = vertexIndices[index][1];
            Vertex v2 = vertexIndices[index][2];

            float oldT = ray.t;
            if(intersectRayWithTriangle(v0.position, v1.position, v2.position, ray, hitInfo)) {
                if(ray.t < oldT) hitInfo.finalTriangleVertices = glm::mat3(v0.position, v1.position, v2.position);
                hitInfo.material = meshes[meshIndices[index]].material;
                hit = true;

                //intersection on the mesh
                glm::vec3 p = ray.origin + ray.t * ray.direction;
                //find the areas of the 3 subtriangles and total triangle area
                float areaTotal = glm::length(glm::cross(v1.position - v0.position, v2.position - v0.position));
                float area0 = glm::length(glm::cross(v1.position - p, v2.position - p));
                float area1 = glm::length(glm::cross(v2.position - p, v0.position - p));
                float area2 = glm::length(glm::cross(v0.position - p, v1.position - p));
                //the weights of the normals
                float w2 = area2 / areaTotal;
                float w0 = area0 / areaTotal;
                float w1 = area1 / areaTotal;

                //normal: addition of all normals with their weights, normalized
                hitInfo.normal = glm::normalize(w2 * v2.normal + w0 * v0.normal + w1 * v1.normal);

                drawRay({ v0.position, v0.normal, 0.1 }, glm::vec3(1, 0, 0));
                drawRay({ v1.position, v1.normal, 0.1 }, glm::vec3(1, 0, 0));
                drawRay({ v2.position, v2.normal, 0.1 }, glm::vec3(1, 0, 0));

                //Textures

                glm::vec2 v0TextCoord = v0.texCoord;
                glm::vec2 v1TextCoord = v1.texCoord;
                glm::vec2 v2TextCoord = v2.texCoord;
                //using barycentric coordinates to find the texture coordinates at the intersection
                glm::vec2 vertexPosTextCoord = w0 * v0TextCoord + w1 * v1TextCoord + w2 * v2TextCoord;

                if (hitInfo.material.kdTexture) {
                    hitInfo.material.kd = hitInfo.material.kdTexture->getTexel(vertexPosTextCoord);
                }
            }
        }
        return hit;
    }

    float originalT = ray.t;

    bool intersectFirst = intersectRayWithShape(AxisAlignedBox{nodes[root.indices[0]].lower, nodes[root.indices[0]].upper}, ray);
    float rayT1 = ray.t;
    ray.t = originalT;

    bool intersectSecond = intersectRayWithShape(AxisAlignedBox{nodes[root.indices[1]].lower, nodes[root.indices[1]].upper}, ray);
    float rayT2 = ray.t;
    ray.t = originalT;

    if(intersectFirst && intersectSecond) {
        //We have to execute both intersect methods to get the closest ray.t
        bool number1 = intersect(nodes, nodes[root.indices[0]], ray, hitInfo, meshIndices, meshes, vertexIndices);
        bool number2 = intersect(nodes, nodes[root.indices[1]], ray, hitInfo, meshIndices, meshes, vertexIndices);

        return number1 || number2;
    }
    if(rayT1 < rayT2) return intersect(nodes, nodes[root.indices[0]], ray, hitInfo, meshIndices, meshes, vertexIndices);
    else return intersect(nodes, nodes[root.indices[1]], ray, hitInfo, meshIndices, meshes, vertexIndices);
}

// Return true if something is hit, returns false otherwise. Only find hits if they are closer than t stored
// in the ray and if the intersection is on the correct side of the origin (the new t >= 0). Replace the code
// by a bounding volume hierarchy acceleration structure as described in the assignment. You can change any
// file you like, including bounding_volume_hierarchy.h.
bool BoundingVolumeHierarchy::intersect(Ray& ray, HitInfo& hitInfo) const {
//    bool hit = false;
//    // Intersect with all triangles of all meshes.
//    for (const auto& mesh : m_pScene->meshes) {
//        for (const auto& tri : mesh.triangles) {
//            const auto v0 = mesh.vertices[tri[0]];
//            const auto v1 = mesh.vertices[tri[1]];
//            const auto v2 = mesh.vertices[tri[2]];
//            if (intersectRayWithTriangle(v0.position, v1.position, v2.position, ray, hitInfo)) {
//                hitInfo.material = mesh.material;
//                hit = true;
//            }
//        }
//    }

    bool hit = false;

    // Intersect with spheres.
    for (const auto& sphere : m_pScene->spheres)
        hit |= intersectRayWithShape(sphere, ray, hitInfo);

    Ray r = ray; //Send a copy over so it doesn't modify the original ray.t
    if(!intersectRayWithShape(AxisAlignedBox{nodes.back().lower, nodes.back().upper}, r)) return false;
    hit |= ::intersect(nodes, nodes.back(), ray, hitInfo, meshIndices, m_pScene->meshes, vertexIndices);
    //drawATriangle(hitInfo.finalTriangleVertices[0], hitInfo.finalTriangleVertices[1], hitInfo.finalTriangleVertices[2]); //Marks the final triangle as blue

    return hit;
}
