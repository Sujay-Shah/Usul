#ifndef __SCENE_SERIALIZER_H__
#define __SCENE_SERIALIZER_H__


#include "Scene.h"

namespace Engine {

	class EditorCamera;

	class SceneSerializer
	{
	public:
		SceneSerializer(const Ref<Scene>& scene);

		void Serialize(const std::string& filepath, EditorCamera* camera = nullptr);
		void SerializeRuntime(const std::string& filepath);

		bool Deserialize(const std::string& filepath, EditorCamera* camera = nullptr);
		bool DeserializeRuntime(const std::string& filepath);
	private:
		Ref<Scene> m_Scene;
	};

}
#endif // !__SCENE_SERIALIZER_H__

