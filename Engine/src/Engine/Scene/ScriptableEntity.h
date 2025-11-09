#ifndef __SCRIPTABLE_ENTITY_H__
#define __SCRIPTABLE_ENTITY_H__

#include "Entity.h"

namespace Engine {

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() {}

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}
	protected:
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(Timestep ts) {}
	private:
		Entity m_Entity;
		friend class Scene;
	};

}

#endif  // __SCRIPTABLE_ENTITY_H__

