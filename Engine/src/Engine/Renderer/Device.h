#pragma once

namespace Engine {
    class Device {
    public:
        virtual ~Device() = default;
        
        virtual void* GetNativeDevice() const = 0;
    };
}
