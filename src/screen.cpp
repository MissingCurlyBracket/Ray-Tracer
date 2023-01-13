#include "screen.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <framework/opengl_includes.h>
#include <string>

Screen::Screen(const glm::ivec2& resolution)
    : m_resolution(resolution)
    , m_textureData(size_t(resolution.x * resolution.y), glm::vec3(0.0f))
{
    // Generate texture
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Screen::clear(const glm::vec3& color)
{
    std::fill(std::begin(m_textureData), std::end(m_textureData), color);
}

void Screen::setPixel(int x, int y, const glm::vec3& color)
{
    // In the window/camera class we use (0, 0) at the bottom left corner of the screen (as used by GLFW).
    // OpenGL / stbi like the origin / (-1,-1) to be at the TOP left corner so transform the y coordinate.
    const int i = (m_resolution.y - 1 - y) * m_resolution.x + x;
    m_textureData[i] = glm::vec4(color, 1.0f);
}

void Screen::writeBitmapToFile(const std::filesystem::path& filePath)
{
    std::vector<glm::u8vec4> textureData8Bits(m_textureData.size());
    std::transform(std::begin(m_textureData), std::end(m_textureData), std::begin(textureData8Bits),
        [](const glm::vec3& color) {
            const glm::vec3 clampedColor = glm::clamp(color, 0.0f, 1.0f);
            return glm::u8vec4(glm::vec4(clampedColor, 1.0f) * 255.0f);
        });

    std::string filePathString = filePath.string();
    stbi_write_bmp(filePathString.c_str(), m_resolution.x, m_resolution.y, 4, textureData8Bits.data());
}

void Screen::draw()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_resolution.x, m_resolution.y, 0, GL_RGB, GL_FLOAT, m_textureData.data());

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_NORMALIZE);
    glColor3f(1.0f, 1.0f, 1.0f);

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(+1.0f, -1.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(+1.0f, +1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-1.0f, +1.0f, 0.0f);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
}
