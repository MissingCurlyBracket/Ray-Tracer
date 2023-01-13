#include "shader.h"
#include "disable_all_warnings.h"
#include <cassert>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static constexpr GLuint invalid = 0xFFFFFFFF;

static bool checkShaderErrors(GLuint shader);
static bool checkProgramErrors(GLuint program);
static std::string readFile(std::filesystem::path filePath);

Shader::Shader(GLuint program)
    : m_program(program)
{
}

Shader::Shader()
    : m_program(invalid)
{
}

Shader::Shader(Shader&& other)
{
    m_program = other.m_program;
    other.m_program = invalid;
}

Shader::~Shader()
{
    if (m_program != invalid)
        glDeleteProgram(m_program);
}

Shader& Shader::operator=(Shader&& other)
{
    if (m_program != invalid)
        glDeleteProgram(m_program);

    m_program = other.m_program;
    other.m_program = invalid;
    return *this;
}

void Shader::bind() const
{
    assert(m_program != invalid);
    glUseProgram(m_program);
}

ShaderBuilder::~ShaderBuilder()
{
    freeShaders();
}

ShaderBuilder& ShaderBuilder::addStage(GLuint shaderStage, std::filesystem::path shaderFile)
{
    if (!std::filesystem::exists(shaderFile)) {
        throw ShaderLoadingException(fmt::format("File {} does not exist", shaderFile.string().c_str()));
    }

    const std::string shaderSource = readFile(shaderFile);
    const GLuint shader = glCreateShader(shaderStage);
    const char* shaderSourcePtr = shaderSource.c_str();
    glShaderSource(shader, 1, &shaderSourcePtr, nullptr);
    glCompileShader(shader);
    if (!checkShaderErrors(shader)) {
        glDeleteShader(shader);
        throw ShaderLoadingException(fmt::format("Failed to compile shader {}", shaderFile.string().c_str()));
    }

    m_shaders.push_back(shader);
    return *this;
}

Shader ShaderBuilder::build()
{
    // Combine vertex and fragment shaders into a single shader program.
    GLuint program = glCreateProgram();
    for (GLuint shader : m_shaders)
        glAttachShader(program, shader);
    glLinkProgram(program);
    freeShaders();

    if (!checkProgramErrors(program)) {
        throw ShaderLoadingException("Shader program failed to link");
    }

    return Shader(program);
}

void ShaderBuilder::freeShaders()
{
    for (GLuint shader : m_shaders)
        glDeleteShader(shader);
}

static std::string readFile(std::filesystem::path filePath)
{
    std::ifstream file(filePath, std::ios::binary);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static bool checkShaderErrors(GLuint shader)
{
    // Check if the shader compiled successfully.
    GLint compileSuccessful;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccessful);

    // If it didn't, then read and print the compile log.
    if (!compileSuccessful) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::string logBuffer;
        logBuffer.resize(static_cast<size_t>(logLength));
        glGetShaderInfoLog(shader, logLength, nullptr, logBuffer.data());

        std::cerr << logBuffer << std::endl;
        return false;
    } else {
        return true;
    }
}

static bool checkProgramErrors(GLuint program)
{
    // Check if the program linked successfully
    GLint linkSuccessful;
    glGetProgramiv(program, GL_LINK_STATUS, &linkSuccessful);

    // If it didn't, then read and print the link log
    if (!linkSuccessful) {
        GLint logLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::string logBuffer;
        logBuffer.resize(static_cast<size_t>(logLength));
        glGetProgramInfoLog(program, logLength, nullptr, logBuffer.data());

        std::cerr << logBuffer << std::endl;
        return false;
    } else {
        return true;
    }
}
