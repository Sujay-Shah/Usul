#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <filesystem>
#include <unordered_map>
#include <string>
#include "Engine/Core/EngineDefines.h"
#include "RHI/rhi.hpp"

namespace Engine {

class Model; // forward-declare to avoid circular include with Model.h

// =============================================================
//  AssetManager  —  synchronous load-once cache
//
//  Usage:
//    rhi::Texture t = AssetManager::GetTexture("textures/albedo.png");
//    Ref<Model>   m = AssetManager::GetModel("models/cube.obj");
//
//  Both functions resolve paths relative to the asset root
//  (ASSET_ROOT cmake variable) and cache the result so that
//  the same resource is never loaded more than once per session.
//  Call Flush() before destroying the RHI device (e.g. OnDetach).
// =============================================================
class AssetManager
{
public:
    static void Init();

    // Path helpers
    static std::filesystem::path GetAssetPath(const std::filesystem::path& relativePath);
    static const std::filesystem::path& GetAssetRoot() { return s_AssetPath; }

    // Texture loading — uploads pixels to the GPU via a staging buffer.
    // Returns a cached handle if the same path was already loaded.
    static rhi::Texture GetTexture(const std::string& relativePath);

    // Model loading — parses via Assimp and uploads all mesh data.
    // Returns a cached shared_ptr.
    static Ref<Model> GetModel(const std::string& relativePath);

    // Destroy all cached GPU resources. Must be called before RHI shutdown.
    static void Flush();

private:
    static std::filesystem::path s_AssetPath;
    static std::unordered_map<std::string, rhi::Texture> s_TextureCache;
    static std::unordered_map<std::string, Ref<Model>>   s_ModelCache;
};

} // namespace Engine

#endif // ASSETMANAGER_H