
#ifndef Vertex_h__
#define Vertex_h__

#include "glm/glm.hpp"

namespace Engine
{
#define MAX_BONE_INFLUENCE 4
	struct Vertex 
	{
		// position
		glm::vec3 Position;
		// normal
		glm::vec3 Normal;
		// texCoords
		glm::vec2 TexCoords;
		// tangent
		glm::vec3 Tangent;
		// bi tangent
		glm::vec3 Bitangent;

		////bone indexes which will influence this vertex
		int m_BoneIDs[MAX_BONE_INFLUENCE];
		////weights from each bone
		float m_Weights[MAX_BONE_INFLUENCE];
	};
}


#endif // Vertex_h__