#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <filesystem>

namespace Engine {

class AssetManager {
public:
    static void Init();
    static std::filesystem::path GetAssetPath(const std::filesystem::path& relativePath);
    static const std::filesystem::path& GetAssetRoot() { return s_AssetPath; }

private:
    static std::filesystem::path s_AssetPath;
};

} // namespace Engine

#endif // ASSETMANAGER_H