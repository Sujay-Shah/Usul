#include "RHIDummy.h"
#include "Engine/RHI/RHI.h"

#include <stdio.h>

namespace Engine
{
    namespace RHI
    {
        void RHIDummy::Init()
        {
            printf("RHIDummy: Init\n");
        }

        void RHIDummy::Shutdown()
        {
            printf("RHIDummy: Shutdown\n");
        }

        Handle RHIDummy::CreateBuffer(const BufferUsage usage, const uint32_t size, const void* data)
        {
            printf("RHIDummy: CreateBuffer\n");
            static uint32_t nextId = 1;
            return { nextId++ };
        }

        Handle RHIDummy::CreateTexture(const TextureFormat format, const uint32_t width, const uint32_t height, const void* data)
        {
            printf("RHIDummy: CreateTexture\n");
            static uint32_t nextId = 1;
            return { nextId++ };
        }

        void RHIDummy::DestroyBuffer(Handle handle)
        {
            printf("RHIDummy: DestroyBuffer\n");
        }

        void RHIDummy::DestroyTexture(Handle handle)
        {
            printf("RHIDummy: DestroyTexture\n");
        }

        void RHIDummy::Submit(const RHI::CommandList& commandList)
        {
            printf("RHIDummy: Submit %zu commands\n", commandList.size());
            for(const auto& cmd : commandList)
            {
                switch(cmd->type)
                {
                    case CommandType::Draw:
                        printf("  CmdDraw\n");
                        break;
                    case CommandType::DrawIndexed:
                        printf("  CmdDrawIndexed\n");
                        break;
                    case CommandType::SetPipeline:
                        printf("  CmdSetPipeline\n");
                        break;
                    case CommandType::SetVertexBuffer:
                        printf("  CmdSetVertexBuffer\n");
                        break;
                    case CommandType::SetIndexBuffer:
                        printf("  CmdSetIndexBuffer\n");
                        break;
                }
            }
        }
        
        void RHIDummy::Present()
        {
            printf("RHIDummy: Present\n");
        }
    }
}
