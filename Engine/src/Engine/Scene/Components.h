#ifndef __COMPONENTS_H__
#define __COMPONENTS_H__

#include <glm/gtx/quaternion.hpp>
#include "SceneCamera.h"
#include <glm/glm.hpp>
#include "ScriptableEntity.h"
#include "RHI/rhi_types.hpp"
namespace Engine {

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{	
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));
			
			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
		SpriteRendererComponent(const glm::vec4& color)
			: Color(color) {}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true; // TODO: think about moving to Scene
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity*(*InstantiateScript)();
		void (*DestroyScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};

	// =========================================================
	//  3D Rendering Components
	// =========================================================

	// Holds GPU mesh data.  Set ModelPath and call AssetManager::GetModel
	// (or build the buffers manually) to populate the buffers.
	struct MeshComponent
	{
		std::string   ModelPath;			// relative to asset root, serialised
		rhi::Buffer   VertexBuffer;
		rhi::Buffer   IndexBuffer;
		uint32_t      VertexCount  = 0;
		uint32_t      IndexCount   = 0;
		bool          CastShadow   = true;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		explicit MeshComponent(const std::string& path) : ModelPath(path) {}
	};

	// PBR material parameters + optional texture overrides.
	// If a texture handle is null (id == 0) the scalar value is used instead.
	struct MaterialComponent
	{
		glm::vec4   AlbedoColor    = { 1.0f, 1.0f, 1.0f, 1.0f };
		float       Metallic       = 0.0f;
		float       Roughness      = 0.5f;
		float       AO             = 1.0f;	// ambient occlusion scalar

		// Optional GPU textures (loaded via AssetManager)
		rhi::Texture  AlbedoMap;
		rhi::Texture  NormalMap;
		rhi::Texture  MetallicRoughnessMap;
		rhi::Texture  AOMap;

		// Texture paths for serialisation
		std::string AlbedoMapPath;
		std::string NormalMapPath;
		std::string MetallicRoughnessMapPath;
		std::string AOMapPath;

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent&) = default;
	};

	enum class LightType : uint32_t
	{
		Directional = 0,
		Point       = 1,
		Spot        = 2
	};

	struct LightComponent
	{
		LightType   Type        = LightType::Directional;
		glm::vec3   Color       = { 1.0f, 1.0f, 1.0f };
		float       Intensity   = 1.0f;
		bool        CastShadows = false;

		// Point / Spot
		float InnerCutoff = glm::radians(12.5f); // Spot inner cone (radians)
		float OuterCutoff = glm::radians(17.5f); // Spot outer cone (radians)
		float Radius      = 10.0f;				 // Point / Spot attenuation radius

		LightComponent() = default;
		LightComponent(const LightComponent&) = default;
	};

} // namespace Engine
#endif