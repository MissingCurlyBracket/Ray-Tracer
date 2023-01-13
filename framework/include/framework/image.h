#pragma once
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <filesystem>
#include <vector>

class Image {
public:
    Image(const std::filesystem::path& filePath);

    glm::vec3 getTexel(const glm::vec2& textureCoordinates) const;

private:
    int m_width, m_height;
    std::vector<glm::vec3> m_pixels;
};
