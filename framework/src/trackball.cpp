// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/gtc/quaternion.hpp>
DISABLE_WARNINGS_POP()
#include <framework/opengl_includes.h>
#include <algorithm>
#include <framework/trackball.h>
#include <framework/window.h>
#include <iostream>
#include <limits>

static constexpr float rotationSpeedFactor = 0.3f;
static constexpr float translationSpeedFactor = 0.005f;
static constexpr float zoomSpeedFactor = 0.5f;

// NOTE(Mathijs): field-of-view in radians.
Trackball::Trackball(Window* pWindow, float fovy, float distFromLookAt, float rotationX, float rotationY)
	: Trackball(pWindow, fovy, glm::vec3(0.0f), distFromLookAt, rotationX, rotationY)
{
}

Trackball::Trackball(Window* pWindow, float fovy, const glm::vec3& lookAt, float distFromLookAt, float rotationX, float rotationY)
	: m_pWindow(pWindow)
	, m_fovy(fovy)
	, m_lookAt(lookAt)
	, m_distanceFromLookAt(distFromLookAt)
	, m_rotationEulerAngles(rotationX, rotationY, 0)
{
	m_rotationEulerAngles.z = 0;

	pWindow->registerMouseButtonCallback(
		[this](int key, int action, int mods) {
			mouseButtonCallback(key, action, mods);
		});
	pWindow->registerMouseMoveCallback(
		[this](const glm::vec2& pos) {
			mouseMoveCallback(pos);
		});
	pWindow->registerScrollCallback(
		[this](const glm::vec2& offset) {
			mouseScrollCallback(offset);
		});
}

void Trackball::printHelp()
{
	std::cout << "Left button: turn in XY," << std::endl;
	std::cout << "Right button: translate in XY," << std::endl;
	std::cout << "Middle button: move along Z." << std::endl;
}

void Trackball::disableTranslation()
{
	m_canTranslate = false;
}

void Trackball::setCamera(const glm::vec3 lookAt, const glm::vec3 rotations, const float dist)
{
	m_lookAt = lookAt;
	m_rotationEulerAngles = rotations;
	m_distanceFromLookAt = dist;
}

glm::vec3 Trackball::position() const
{
	return m_lookAt + glm::quat(m_rotationEulerAngles) * glm::vec3(0, 0, -m_distanceFromLookAt);
}

glm::vec3 Trackball::lookAt() const
{
	return m_lookAt;
}

glm::mat4 Trackball::viewMatrix() const
{
	return glm::lookAt(position(), m_lookAt, up());
}

glm::mat4 Trackball::projectionMatrix() const
{
	return glm::perspective(m_fovy, m_pWindow->getAspectRatio(), 0.01f, 100.0f);
}

// Generate a ray with the origin at cameraPos, going through the given pixel (normalized coordinates between -1 and +1)
// on the virtual image plane in front of the camera.
Ray Trackball::generateRay(const glm::vec2& pixel) const
{
	const float halfScreenPlaceHeight = std::tan(m_fovy / 2.0f);
	const float halfScreenPlaceWidth = m_pWindow->getAspectRatio() * halfScreenPlaceHeight;
	const glm::vec3 cameraSpaceDirection = glm::normalize(glm::vec3(-pixel.x * halfScreenPlaceWidth, pixel.y * halfScreenPlaceHeight, 1.0f));

	Ray ray;
	ray.origin = position();
	ray.direction = glm::quat(m_rotationEulerAngles) * cameraSpaceDirection;
	ray.t = std::numeric_limits<float>::max();
	return ray;
}

glm::vec3 Trackball::forward() const
{
	return glm::quat(m_rotationEulerAngles) * glm::vec3(0, 0, 1);
}

glm::vec3 Trackball::up() const
{
	return glm::quat(m_rotationEulerAngles) * glm::vec3(0, 1, 0);
}

glm::vec3 Trackball::left() const
{
	// NOTE(Mathijs): positive X is to the right because of the right-handed coordinate system in OpenGL.
	return glm::quat(m_rotationEulerAngles) * glm::vec3(1, 0, 0);
}

void Trackball::mouseButtonCallback(int button, int action, int /* mods */)
{

	if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_PRESS) {
		m_prevCursorPos = m_pWindow->getCursorPos();
	}
}

void Trackball::mouseMoveCallback(const glm::vec2& pos)
{
	const bool rotateXY = m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
	const bool translateXY = m_canTranslate && m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT);

	if (rotateXY || translateXY) {
		// Amount of cursor motion compared to the previous frame. Positive = right/top
		const glm::vec2 delta = pos - m_prevCursorPos;

		if (rotateXY) {
			// Rotate the camera around the lookat point.
			m_rotationEulerAngles.x = std::clamp(m_rotationEulerAngles.x - glm::radians(delta.y * rotationSpeedFactor), -glm::half_pi<float>(), +glm::half_pi<float>());
			m_rotationEulerAngles.y -= glm::radians(delta.x * rotationSpeedFactor);

		}
		else {
			// Translate the camera in the image plane.
			m_lookAt += delta.x * translationSpeedFactor * left(); // Mouse right => camera left
			m_lookAt -= delta.y * translationSpeedFactor * up(); // Mouse up => camera down
		}
		m_prevCursorPos = pos;
	}
}

void Trackball::mouseScrollCallback(const glm::vec2& offset)
{
	// Move the camera closer/further from the look at point when the user scrolls on his/her mousewheel.
	m_distanceFromLookAt += -float(offset.y) * zoomSpeedFactor;
	m_distanceFromLookAt = std::clamp(m_distanceFromLookAt, 0.1f, 100.0f);
}
