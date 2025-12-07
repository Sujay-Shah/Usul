#ifndef __MATH_H__
#define __MATH_H__



#include <glm/glm.hpp>

namespace Engine::Math {

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

}

#endif
