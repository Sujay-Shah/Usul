#include "EnginePCH.h"
#include "Math.h"
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace Engine::Math {

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale)
	{
		// From glm::decompose in matrix_decompose.inl

		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(transform, scale, orientation, translation, skew, perspective);

		rotation = glm::eulerAngles(orientation);
		return true;
	}

}
