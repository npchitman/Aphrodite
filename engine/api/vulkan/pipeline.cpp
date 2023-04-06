#include "pipeline.h"
#include "scene/mesh.h"

namespace aph
{
namespace
{
std::unordered_map<VertexComponent, VkVertexInputAttributeDescription> vertexComponmentMap{
    { VertexComponent::POSITION, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) } },
    { VertexComponent::NORMAL, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) } },
    { VertexComponent::UV, { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) } },
    { VertexComponent::COLOR, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) } },
    { VertexComponent::TANGENT, { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) } },
};
}

VkPipelineVertexInputStateCreateInfo &VertexInputBuilder::getPipelineVertexInputState(
    const std::vector<VertexComponent> &components)
{
    uint32_t location = 0;
    for(VertexComponent component : components)
    {
        VkVertexInputAttributeDescription desc = vertexComponmentMap[component];
        desc.location = location;
        inputAttribute.push_back(desc);
        location++;
    }
    inputBinding = { { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX } };
    vertexInputState = aph::init::pipelineVertexInputStateCreateInfo(
        inputBinding, inputAttribute);
    return vertexInputState;
}

VulkanPipeline *VulkanPipeline::CreateGraphicsPipeline(VulkanDevice *pDevice,
                                                       const GraphicsPipelineCreateInfo &createInfo,
                                                       VkRenderPass renderPass, VkPipelineLayout layout,
                                                       VkPipeline handle)
{
    auto *instance = new VulkanPipeline();
    instance->getHandle() = handle;
    instance->m_device = pDevice;
    instance->m_layout = layout;
    instance->m_setLayouts = createInfo.setLayouts;
    instance->m_constants = createInfo.constants;
    instance->m_shaderMapList = createInfo.shaderMapList;
    instance->m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    return instance;
}

VulkanPipeline *VulkanPipeline::CreateComputePipeline(VulkanDevice *pDevice,
                                                      const ComputePipelineCreateInfo &createInfo, VkPipelineLayout layout, VkPipeline handle)
{
    auto *instance = new VulkanPipeline();
    instance->getHandle() = handle;
    instance->m_device = pDevice;
    instance->m_layout = layout;
    instance->m_setLayouts = createInfo.setLayouts;
    instance->m_constants = createInfo.constants;
    instance->m_shaderMapList = createInfo.shaderMapList;
    instance->m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    return instance;
}

}  // namespace aph
