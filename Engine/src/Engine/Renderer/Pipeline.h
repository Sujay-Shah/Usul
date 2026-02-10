#pragma once

namespace Engine {
    class Pipeline {
    public:
        virtual ~Pipeline() = default;

        virtual void Create() = 0;
        virtual void Cleanup() = 0;
    };
}
