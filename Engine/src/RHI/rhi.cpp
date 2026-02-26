#include "rhi.hpp"
#include <cstdio>
#include <cassert>

// Each compiled-in backend exposes one function.
// Guard with feature macros so unused backends don't link.
#define RHI_ENABLE_VULKAN

#ifdef RHI_ENABLE_VULKAN
namespace vkrhi { const rhi::BackendApi* get_api(); }
#endif
#ifdef RHI_ENABLE_DX12
namespace dx12rhi { const rhi::BackendApi* get_api(); }
#endif

namespace rhi {

const BackendApi* g_rhi = nullptr;

bool init(const InitDesc& desc)
{
    assert(!g_rhi && "rhi::init() called twice");

    switch (desc.backend)
    {
#ifdef RHI_ENABLE_VULKAN
        case Backend::Vulkan:
            g_rhi = vkrhi::get_api();
            break;
#endif
#ifdef RHI_ENABLE_DX12
        case Backend::DX12:
            g_rhi = dx12rhi::get_api();
            break;
#endif
        default:
            fprintf(stderr, "[rhi] Requested backend not compiled in.\n");
            return false;
    }

    if (!g_rhi->init(desc))
    {
        fprintf(stderr, "[rhi] Backend init failed.\n");
        g_rhi = nullptr;
        return false;
    }
    return true;
}

void shutdown()
{
    if (g_rhi)
    {
        g_rhi->shutdown();
        g_rhi = nullptr;
    }
}

} // namespace rhi
