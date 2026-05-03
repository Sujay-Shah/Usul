#ifndef __SCENE_H__
#define __SCENE_H__

#include <entt.hpp>
#include "Engine/Core/Timestep.h"
#include "Engine/Renderer/Camera/EditorCamera.h"

namespace Engine {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);
		void OnViewportResize(uint32_t width, uint32_t height);
		void DestroyEntity(Entity entity);
		Entity GetPrimaryCameraEntity();

		// Allow SceneRenderer and serializer to iterate components
		entt::registry& GetRegistry() { return m_Registry; }
		const entt::registry& GetRegistry() const { return m_Registry; }
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);

		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class SceneSerializer;
		friend class SceneRenderer;
	};

}
#endif  // __SCENE_H__
