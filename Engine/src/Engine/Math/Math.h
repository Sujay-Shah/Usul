#ifndef __USUL_ENGINE_MATH_H__
#define __USUL_ENGINE_MATH_H__



#include <glm/glm.hpp>

namespace Engine::Math {

	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::vec3& outRotation, glm::vec3& outScale);

}

#endif
