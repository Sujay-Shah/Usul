#ifndef __SCENE_HIERARCHY_PANEL_H__
#define __SCENE_HIERARCHY_PANEL_H__


#include "Engine/Core/EngineDefines.h"
#include "Engine/Core/Logging.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"

namespace Engine {

	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);

		void OnImGuiRender();
	private:
		void DrawEntityNode(Entity entity);
	private:
		Ref<Scene> m_Context;
		Entity m_SelectionContext;
	};

}

#endif // !__SCENE_HIERARCHY_PANEL_H__

