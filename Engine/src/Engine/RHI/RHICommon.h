#pragma once

#include <cstdint>

namespace Engine
{
    namespace RHI
    {
        // Opaque handle for all RHI resources.
        struct Handle
        {
            uint32_t id = 0;

            bool IsValid() const { return id != 0; }
            bool operator==(const Handle& other) const { return id == other.id; }
            bool operator!=(const Handle& other) const { return id != other.id; }
        };

        // Null handle
        const Handle NullHandle = { 0 };

        // Resource Types
        enum class ResourceType : uint8_t
        {
            Buffer,
            Texture,
            Sampler,
            Shader,
            Pipeline,
            RenderTarget,
            Count
        };

        enum class PrimitiveTopology : uint8_t
        {
            Triangles,
            Lines,
            Points
        };

        // Buffer Usage
        enum class BufferUsage : uint8_t
        {
            Vertex,
            Index,
            Uniform,
            Storage
        };

        // Texture Format
        enum class TextureFormat : uint8_t
        {
            R8,
            RG8,
            RGB8,
            RGBA8,
            R16F,
            RG16F,
            RGB16F,
            RGBA16F,
            R32F,
            RG32F,
            RGB32F,
            RGBA32F,
            Depth32F,
            Depth24Stencil8
        };
        
        enum class ShaderStage : uint8_t
        {
            Vertex,
            Fragment,
            Compute
        };
    }
}
