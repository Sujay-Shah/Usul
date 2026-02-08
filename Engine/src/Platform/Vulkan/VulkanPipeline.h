#pragma once

#include "Engine/RHI/Pipeline.h"
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include "VulkanDevice.h"

namespace Engine {

    class VulkanPipeline : public Pipeline {
    public:
        VulkanPipeline(const std::shared_ptr<VulkanDevice>& device, const std::string& vertShaderPath, const std::string& fragShaderPath, VkFormat colorAttachmentFormat);
        ~VulkanPipeline();

        void Create() override;
        void Cleanup() override;

        VkPipeline getPipeline() const { return m_graphicsPipeline; }
        VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
        VkRenderPass getRenderPass() const { return m_renderPass; }

    private:
        static std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);

        void createRenderPass(VkFormat colorAttachmentFormat);

    private:
        std::shared_ptr<VulkanDevice> m_device;
        std::string m_vertShaderPath;
        std::string m_fragShaderPath;

        VkPipeline m_graphicsPipeline;
        VkPipelineLayout m_pipelineLayout;
        VkRenderPass m_renderPass;
    };
}
