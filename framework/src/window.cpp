#include "window.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <imgui.h>
#undef IMGUI_IMPL_OPENGL_LOADER_GLEW
#define IMGUI_IMPL_OPENGL_LOADER_GLAD 1
#include "imgui_impl_opengl3.h"
#include <iostream>

static void glfwErrorCallback(int error, const char* description)
{
    std::cerr << "GLFW error code: " << error << std::endl;
    std::cerr << description << std::endl;
    exit(1);
}

#ifdef GL_DEBUG_SEVERITY_NOTIFICATION
// OpenGL debug callback
void APIENTRY glDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION && type != GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) {
        std::cerr << "OpenGL: " << message << std::endl;
    }
}
#endif

Window::Window(std::string_view title, const glm::ivec2& windowSize, OpenGLVersion glVersion)
    : m_glVersion(glVersion)
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW" << std::endl;
        exit(1);
    }

    if (glVersion == OpenGLVersion::GL3) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else if (glVersion == OpenGLVersion::GL45) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }
#ifndef NDEBUG // Automatically defined by CMake when compiling in Release/MinSizeRel mode.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif
    
    // HighDPI awareness
    // https://decovar.dev/blog/2019/08/04/glfw-dear-imgui/#high-dpi
#ifdef _WIN32
    // if it's a HighDPI monitor, try to scale everything
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    float xscale, yscale;
    glfwGetMonitorContentScale(monitor, &xscale, &yscale);
    if (xscale > 1 || yscale > 1)
    {
        m_dpiScalingFactor = xscale;
        glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    }
#elif __APPLE__
    // to prevent 1200x800 from becoming 2400x1600
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif


    // std::string_view does not guarantee that the string contains a terminator character.
    const std::string titleString { title };
    m_pWindow = glfwCreateWindow(windowSize.x, windowSize.y, titleString.c_str(), nullptr, nullptr);
    if (m_pWindow == nullptr) {
        glfwTerminate();
        std::cerr << "Could not create GLFW window" << std::endl;
        exit(1);
    }
    glfwMakeContextCurrent(m_pWindow);
    glfwSwapInterval(1); // Enable vsync. To disable vsync set this to 0.

    float xScale, yScale;
    glfwGetWindowContentScale(m_pWindow, &xScale, &yScale);
    std::cout << "Window content scale: " << xScale << ", " << yScale << std::endl;

    glfwGetWindowSize(m_pWindow, &m_windowSize.x, &m_windowSize.y);

    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        std::cerr << "Could not initialize GLEW" << std::endl;
        exit(1);
    }
    int glVersionMajor, glVersionMinor;
    glGetIntegerv(GL_MAJOR_VERSION, &glVersionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glVersionMinor);
    std::cout << "Initialized OpenGL version " << glVersionMajor << "." << glVersionMinor << std::endl;

#if defined(GL_DEBUG_SEVERITY_NOTIFICATION) && !defined(NDEBUG)
    // Custom debug message with breakpoints at the exact error. Only supported on OpenGL 4.1 and higher.
    if (glVersionMajor > 4 || (glVersionMajor == 4 && glVersionMinor >= 3)) {
        // Set OpenGL debug callback when supported (OpenGL 4.3).
        // NOTE(Mathijs): this is not supported on macOS since Apple can't be bothered to update
        //  their OpenGL version past 4.1 which released 10 years ago!
        glDebugMessageCallback(glDebugCallback, nullptr);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
#endif

    // Setup Dear ImGui context.
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    // Setup Dear ImGui style.
    ImGui::StyleColorsDark();
    // ImGui scaling for HighDPI displays
    // https://twitter.com/ocornut/status/939547856171659264?lang=en
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(m_dpiScalingFactor);
    ImFontConfig cfg;
    cfg.SizePixels = 13 * m_dpiScalingFactor;
    io.Fonts->AddFontDefault(&cfg);

    ImGui_ImplGlfw_InitForOpenGL(m_pWindow, true);
    switch (glVersion) {
    case OpenGLVersion::GL2: {
        if (!ImGui_ImplOpenGL2_Init()) {
            std::cerr << "Could not initialize imgui" << std::endl;
            exit(1);
        }
    } break;
    case OpenGLVersion::GL3:
    case OpenGLVersion::GL45: {
        if (!ImGui_ImplOpenGL3_Init()) {
            std::cerr << "Could not initialize imgui" << std::endl;
            exit(1);
        }
    } break;
    };

    glfwSetWindowUserPointer(m_pWindow, this);

    glfwSetKeyCallback(m_pWindow, keyCallback);
    glfwSetCharCallback(m_pWindow, charCallback);
    glfwSetMouseButtonCallback(m_pWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(m_pWindow, mouseMoveCallback);
    glfwSetScrollCallback(m_pWindow, scrollCallback);
    glfwSetWindowSizeCallback(m_pWindow, windowSizeCallback);
}

Window::~Window()
{
    switch (m_glVersion) {
    case OpenGLVersion::GL2: {
        ImGui_ImplOpenGL2_Shutdown();
    } break;
    case OpenGLVersion::GL3:
    case OpenGLVersion::GL45: {
        ImGui_ImplOpenGL3_Shutdown();
    } break;
    };
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_pWindow);
    glfwTerminate();
}

void Window::close()
{
    glfwSetWindowShouldClose(m_pWindow, 1);
}

bool Window::shouldClose()
{
    return glfwWindowShouldClose(m_pWindow) != 0;
}

void Window::updateInput()
{
    glfwPollEvents();

    // Start the Dear ImGui frame.
    switch (m_glVersion) {
    case OpenGLVersion::GL2: {
        ImGui_ImplOpenGL2_NewFrame();
    } break;
    case OpenGLVersion::GL3:
    case OpenGLVersion::GL45: {
        ImGui_ImplOpenGL3_NewFrame();
    } break;
    };
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Window::swapBuffers()
{
    // Rendering of Dear ImGui ui.
    ImGui::Render();
    switch (m_glVersion) {
    case OpenGLVersion::GL2: {
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    } break;
    case OpenGLVersion::GL3:
    case OpenGLVersion::GL45: {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    } break;
    };

    glfwSwapBuffers(m_pWindow);
}

void Window::registerKeyCallback(KeyCallback&& callback)
{
    m_keyCallbacks.push_back(std::move(callback));
}

void Window::registerCharCallback(CharCallback&& callback)
{
    m_charCallbacks.push_back(std::move(callback));
}

void Window::registerMouseButtonCallback(MouseButtonCallback&& callback)
{
    m_mouseButtonCallbacks.push_back(std::move(callback));
}

void Window::registerScrollCallback(ScrollCallback&& callback)
{
    m_scrollCallbacks.push_back(std::move(callback));
}

void Window::registerWindowResizeCallback(WindowResizeCallback&& callback)
{
    m_windowResizeCallbacks.push_back(std::move(callback));
}

void Window::registerMouseMoveCallback(MouseMoveCallback&& callback)
{
    m_mouseMoveCallbacks.push_back(std::move(callback));
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

    // Ignore callbacks when the user is interacting with imgui.
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_keyCallbacks)
        callback(key, scancode, action, mods);
}

void Window::charCallback(GLFWwindow* window, unsigned unicodeCodePoint)
{
    ImGui_ImplGlfw_CharCallback(window, unicodeCodePoint);

    // Ignore callbacks when the user is interacting with imgui.
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    
    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_charCallbacks)
        callback(unicodeCodePoint);
}

void Window::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // Ignore callbacks when the user is interacting with imgui.
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_mouseButtonCallbacks)
        callback(button, action, mods);
}

void Window::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Ignore callbacks when the user is interacting with imgui.
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_mouseMoveCallbacks)
        callback(glm::vec2(xpos, pThisWindow->m_windowSize.y - 1 - ypos));
}

void Window::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Ignore callbacks when the user is interacting with imgui.
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    const Window* pThisWindow = static_cast<const Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : pThisWindow->m_scrollCallbacks)
        callback(glm::vec2(xoffset, yoffset));
}

void Window::windowSizeCallback(GLFWwindow* window, int width, int height)
{
    Window* pThisWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    pThisWindow->m_windowSize = glm::ivec2 { width, height };

    for (const auto& callback : pThisWindow->m_windowResizeCallbacks)
        callback(glm::ivec2(width, height));
}

bool Window::isKeyPressed(int key) const
{
    return glfwGetKey(m_pWindow, key) == GLFW_PRESS;
}

bool Window::isMouseButtonPressed(int button) const
{
    return glfwGetMouseButton(m_pWindow, button) == GLFW_PRESS;
}

glm::vec2 Window::getCursorPos() const
{
    double x, y;
    glfwGetCursorPos(m_pWindow, &x, &y);
    return glm::vec2(x, m_windowSize.y - 1 - y);
}

glm::vec2 Window::getNormalizedCursorPos() const
{
    return getCursorPos() / glm::vec2(m_windowSize);
}

glm::vec2 Window::getCursorPixel() const
{
    // https://stackoverflow.com/questions/45796287/screen-coordinates-to-world-coordinates
    // Coordinates returned by glfwGetCursorPos are in screen coordinates which may not map 1:1 to
    // pixel coordinates on some machines (e.g. with resolution scaling).
    glm::ivec2 screenSize;
    glfwGetWindowSize(m_pWindow, &screenSize.x, &screenSize.y);
    glm::ivec2 framebufferSize;
    glfwGetFramebufferSize(m_pWindow, &framebufferSize.x, &framebufferSize.y);

    //double xpos, ypos;
    //glfwGetCursorPos(m_pWindow, &xpos, &ypos);
    //const glm::vec2 screenPos{ xpos, ypos };
    const glm::vec2 screenPos = getCursorPos();
    glm::vec2 pixelPos = screenPos / glm::vec2(screenSize) * glm::vec2(framebufferSize); // float division
    pixelPos += glm::vec2(0.5f); // Shift to GL center convention.
    return glm::vec2(pixelPos.x, pixelPos.y);
}

void Window::setMouseCapture(bool capture)
{
    if (capture) {
        glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    glfwPollEvents();
}

glm::ivec2 Window::getWindowSize() const
{
    return m_windowSize;
}

glm::ivec2 Window::getFrameBufferSize() const
{
    glm::ivec2 out{};
    glfwGetFramebufferSize(m_pWindow, &out.x, &out.y);
    return out;
}

float Window::getAspectRatio() const
{
    if (m_windowSize.x == 0 || m_windowSize.y == 0)
        return 1.0f;
    return float(m_windowSize.x) / float(m_windowSize.y);
}

float Window::getDpiScalingFactor() const
{
    return m_dpiScalingFactor;
}
