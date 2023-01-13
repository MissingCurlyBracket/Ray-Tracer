#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/trackball.h>
#include <framework/window.h>

void showImGuizmoTranslation(const Window& window, const Trackball& camera, glm::vec3& position);
