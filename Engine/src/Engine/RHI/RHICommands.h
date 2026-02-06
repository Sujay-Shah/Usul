#pragma once

#include "RHICommon.h"

namespace Engine
{
    namespace RHI
    {
        enum class CommandType : uint8_t
        {
            Draw,
            DrawIndexed,
            SetPipeline,
            SetVertexBuffer,
            SetIndexBuffer,
        };

        struct CommandBase
        {
            CommandType type;
        };

        struct CmdDraw : public CommandBase
        {
            CmdDraw() { type = CommandType::Draw; }
            uint32_t vertexCount;
            uint32_t instanceCount;
            uint32_t firstVertex;
            uint32_t firstInstance;
        };

        struct CmdDrawIndexed : public CommandBase
        {
            CmdDrawIndexed() { type = CommandType::DrawIndexed; }
            uint32_t indexCount;
            uint32_t instanceCount;
            uint32_t firstIndex;
            int32_t  vertexOffset;
            uint32_t firstInstance;
        };

        struct CmdSetPipeline : public CommandBase
        {
            CmdSetPipeline() { type = CommandType::SetPipeline; }
            Handle pipeline;
        };

        struct CmdSetVertexBuffer : public CommandBase
        {
            CmdSetVertexBuffer() { type = CommandType::SetVertexBuffer; }
            uint32_t slot;
            Handle buffer;
            uint64_t offset;
        };

        struct CmdSetIndexBuffer : public CommandBase
        {
            CmdSetIndexBuffer() { type = CommandType::SetIndexBuffer; }
            Handle buffer;
            uint64_t offset;
        };

        // A command list is just a sequence of commands in a buffer.
        // For simplicity, we can use a std::vector for now.
        // A more optimized implementation would use a custom allocator.
        // The command list itself is not a command. It is a container for commands.
        // We will need a way to build these lists.
    }
}
