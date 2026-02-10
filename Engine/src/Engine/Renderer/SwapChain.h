#pragma once

#include <cstdint>

namespace Engine {
    class SwapChain {
    public:
        virtual ~SwapChain() = default;

        virtual void Create() = 0;
        virtual void Cleanup() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
    };
}
