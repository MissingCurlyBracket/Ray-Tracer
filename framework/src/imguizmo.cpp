#include "imguizmo.h"
#include "disable_all_warnings.h"
DISABLE_WARNINGS_PUSH()
#include "ImGuizmo/ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()

void showImGuizmoTranslation(const Window& window, const Trackball& camera, glm::vec3& position)
{
	ImGuizmo::SetGizmoSizeClipSpace(0.15f * window.getDpiScalingFactor());
	ImGuizmo::SetRect(0.0f, 0.0f, (float)window.getWindowSize().x, (float)window.getWindowSize().y);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
	const glm::mat4 viewMatrix = camera.viewMatrix();
	const glm::mat4 projectionMatrix = camera.projectionMatrix();
	glm::mat4 modelMatrix = glm::translate(glm::identity<glm::mat4>(), position);
	const bool manipulated = ImGuizmo::Manipulate(
		glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix),
		ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::LOCAL, glm::value_ptr(modelMatrix));

	if (manipulated) {
		glm::vec3 dummyScale, dummyRotation; // Unused; DecomposeMatrixToComponents does not support passing nullptr.
		ImGuizmo::DecomposeMatrixToComponents(
			glm::value_ptr(modelMatrix), glm::value_ptr(position), glm::value_ptr(dummyRotation), glm::value_ptr(dummyScale));
	}
}
