#include "AssetManager.h"
#include "Engine/Core/Logging.h"
#include "Engine/Renderer/Model.h"
#include "RHI/rhi.hpp"

// stb_image for texture decoding
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Defined by CMake — absolute path to the assets directory.
#ifndef ASSET_ROOT
#define ASSET_ROOT ""
#endif

namespace Engine {

// ---- static member definitions ----
std::filesystem::path                              AssetManager::s_AssetPath;
std::unordered_map<std::string, rhi::Texture>     AssetManager::s_TextureCache;
std::unordered_map<std::string, Ref<Model>>       AssetManager::s_ModelCache;

// ---- Init ----
void AssetManager::Init()
{
    s_AssetPath = ASSET_ROOT;
    if (!std::filesystem::exists(s_AssetPath))
        ENGINE_CORE_ERROR("Asset root path does not exist: {0}", s_AssetPath.string());
    else
        ENGINE_CORE_INFO("Asset root path: {0}", s_AssetPath.string());
}

// ---- Path helper ----
std::filesystem::path AssetManager::GetAssetPath(const std::filesystem::path& relativePath)
{
    std::string pathStr = relativePath.string();
    if (pathStr.rfind("assets/", 0) == 0) // starts_with
    {
        pathStr = pathStr.substr(7);
    }
    else if (pathStr.rfind("assets\\", 0) == 0)
    {
        pathStr = pathStr.substr(7);
    }
    return (s_AssetPath / pathStr).lexically_normal();
}

// ---- Texture loading ----
rhi::Texture AssetManager::GetTexture(const std::string& relativePath)
{
    auto it = s_TextureCache.find(relativePath);
    if (it != s_TextureCache.end())
        return it->second;

    std::string absPath = GetAssetPath(relativePath).string();

    int width = 0, height = 0, channels = 0;
    // Force 4-channel RGBA so format is always RGBA8_Unorm
    stbi_uc* pixels = stbi_load(absPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        ENGINE_CORE_ERROR("AssetManager: failed to load texture '{0}': {1}", absPath, stbi_failure_reason());
        return {};
    }

    uint64_t size = (uint64_t)width * height * 4;

    rhi::Texture tex = rhi::texture_create({
        .width  = (uint32_t)width,
        .height = (uint32_t)height,
        .format = rhi::Format::RGBA8_Unorm,
        .usage  = rhi::TextureUsage::Sampled | rhi::TextureUsage::TransferDst,
        .name   = relativePath.c_str(),
    });

    // Upload via a temporary staging buffer
    rhi::UploadContext ctx = rhi::UploadContext::create(size);
    ctx.begin();
    ctx.upload_texture(tex, pixels, size, (uint32_t)width, (uint32_t)height);
    ctx.submit_and_wait();
    ctx.destroy();

    stbi_image_free(pixels);

    ENGINE_CORE_INFO("AssetManager: loaded texture '{0}' ({1}x{2})", relativePath, width, height);
    s_TextureCache[relativePath] = tex;
    return tex;
}

// ---- Model loading ----
Ref<Model> AssetManager::GetModel(const std::string& relativePath)
{
    auto it = s_ModelCache.find(relativePath);
    if (it != s_ModelCache.end())
        return it->second;

    // Model constructor resolves the path internally via AssetManager::GetAssetPath
    Ref<Model> model = CreateRef<Model>(relativePath);
    ENGINE_CORE_INFO("AssetManager: loaded model '{0}'", relativePath);
    s_ModelCache[relativePath] = model;
    return model;
}

// ---- Flush (call before RHI shutdown) ----
void AssetManager::Flush()
{
    for (auto& [path, tex] : s_TextureCache)
    {
        if (tex)
            rhi::texture_destroy(tex);
    }
    s_TextureCache.clear();
    s_ModelCache.clear();  // Models hold rhi::Buffers; Mesh destructors must clean those up.
    ENGINE_CORE_INFO("AssetManager: flushed all cached resources.");
}

} // namespace Engine