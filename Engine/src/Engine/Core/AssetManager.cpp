#include "AssetManager.h"
#include "Engine/Core/Logging.h"

// This will be defined by CMake. It should be the absolute path to the assets directory.
#ifndef ASSET_ROOT
#define ASSET_ROOT ""
#endif

namespace Engine {

std::filesystem::path AssetManager::s_AssetPath;

void AssetManager::Init() {
    s_AssetPath = ASSET_ROOT;
    if (!std::filesystem::exists(s_AssetPath)) {
        ENGINE_CORE_ERROR("Asset root path does not exist: {0}", s_AssetPath.string());
    } else {
        ENGINE_CORE_INFO("Asset root path: {0}", s_AssetPath.string());
    }
}

std::filesystem::path AssetManager::GetAssetPath(const std::filesystem::path& relativePath) {
    return (s_AssetPath / relativePath).lexically_normal();
}

} // namespace Engine