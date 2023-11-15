#ifndef PIPELINE_H_
#define PIPELINE_H_

#include "vkUtils.h"

namespace aph::vk
{
class Device;
class DescriptorSet;
class Sampler;
class ShaderProgram;
class Shader;
struct ImmutableSamplerBank;

enum
{
    APH_MAX_COLOR_ATTACHMENTS = 5
};
enum
{
    APH_MAX_MIP_LEVELS = 16
};

struct ColorAttachment
{
    Format        format              = Format::Undefined;
    bool          blendEnabled        = false;
    VkBlendOp     rgbBlendOp          = VK_BLEND_OP_ADD;
    VkBlendOp     alphaBlendOp        = VK_BLEND_OP_ADD;
    VkBlendFactor srcRGBBlendFactor   = VK_BLEND_FACTOR_ONE;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstRGBBlendFactor   = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    bool operator==(const ColorAttachment& rhs) const
    {
        return format == rhs.format && blendEnabled == rhs.blendEnabled && rgbBlendOp == rhs.rgbBlendOp &&
               alphaBlendOp == rhs.alphaBlendOp && srcRGBBlendFactor == rhs.srcRGBBlendFactor &&
               srcAlphaBlendFactor == rhs.srcAlphaBlendFactor && dstRGBBlendFactor == rhs.dstAlphaBlendFactor &&
               dstAlphaBlendFactor == rhs.dstAlphaBlendFactor;
    }
};

struct StencilState
{
    VkStencilOp stencilFailureOp   = VK_STENCIL_OP_KEEP;
    VkStencilOp depthFailureOp     = VK_STENCIL_OP_KEEP;
    VkStencilOp depthStencilPassOp = VK_STENCIL_OP_KEEP;
    VkCompareOp stencilCompareOp   = VK_COMPARE_OP_ALWAYS;
    uint32_t    readMask           = (uint32_t)~0;
    uint32_t    writeMask          = (uint32_t)~0;

    bool operator==(const StencilState& rhs) const
    {
        return stencilFailureOp == rhs.stencilFailureOp && stencilCompareOp == rhs.stencilCompareOp &&
               depthStencilPassOp == rhs.depthStencilPassOp && depthFailureOp == rhs.depthFailureOp &&
               readMask == rhs.readMask && writeMask == rhs.writeMask;
    }
};

struct RenderPipelineDynamicState final
{
    VkBool32 depthBiasEnable = VK_FALSE;

    bool operator==(const RenderPipelineDynamicState& rhs) const { return depthBiasEnable == rhs.depthBiasEnable; }
};

struct GraphicsPipelineCreateInfo
{
    RenderPipelineDynamicState dynamicState = {};
    VkPrimitiveTopology        topology     = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VertexInput vertexInput;

    ShaderProgram* pProgram = {};

    ImmutableSamplerBank* pSamplerBank = {};

    std::vector<ColorAttachment> color         = {};
    Format                       depthFormat   = Format::Undefined;
    Format                       stencilFormat = Format::Undefined;

    VkCullModeFlags cullMode         = VK_CULL_MODE_NONE;
    VkFrontFace     frontFaceWinding = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPolygonMode   polygonMode      = VK_POLYGON_MODE_FILL;

    StencilState backFaceStencil  = {};
    StencilState frontFaceStencil = {};

    uint32_t samplesCount = 1u;

    bool operator==(const GraphicsPipelineCreateInfo& rhs) const
    {
        return dynamicState == rhs.dynamicState && topology == rhs.topology && vertexInput == rhs.vertexInput &&
               pProgram == rhs.pProgram && pSamplerBank == rhs.pSamplerBank && depthFormat == rhs.depthFormat &&
               stencilFormat == rhs.stencilFormat && polygonMode == rhs.polygonMode &&
               backFaceStencil == rhs.backFaceStencil && frontFaceStencil == rhs.frontFaceStencil &&
               frontFaceWinding == rhs.frontFaceWinding && samplesCount == rhs.samplesCount;
    }
};

struct RenderPipelineState
{
    GraphicsPipelineCreateInfo createInfo;

    std::vector<VkVertexInputBindingDescription>   vkBindings   = {};
    std::vector<VkVertexInputAttributeDescription> vkAttributes = {};

    // non-owning, cached the last pipeline layout from the context (if the context has a new layout, invalidate all
    // VkPipeline objects)
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    // [depthBiasEnable]
    VkPipeline pipelines[2] = {};
};

struct ComputePipelineCreateInfo
{
    ImmutableSamplerBank* pSamplerBank = {};
    ShaderProgram*        pCompute     = {};

    bool operator==(const ComputePipelineCreateInfo& rhs) const
    {
        return pSamplerBank == rhs.pSamplerBank && pCompute == rhs.pCompute;
    }
};

class VulkanPipelineBuilder final
{
public:
    VulkanPipelineBuilder();
    ~VulkanPipelineBuilder() = default;

    VulkanPipelineBuilder& depthBiasEnable(bool enable);
    VulkanPipelineBuilder& dynamicState(VkDynamicState state);
    VulkanPipelineBuilder& primitiveTopology(VkPrimitiveTopology topology);
    VulkanPipelineBuilder& rasterizationSamples(VkSampleCountFlagBits samples);
    VulkanPipelineBuilder& shaderStage(VkPipelineShaderStageCreateInfo stage);
    VulkanPipelineBuilder& shaderStage(const SmallVector<VkPipelineShaderStageCreateInfo>& stages);
    VulkanPipelineBuilder& stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp,
                                           VkStencilOp depthFailOp, VkCompareOp compareOp);
    VulkanPipelineBuilder& stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask, uint32_t writeMask,
                                        uint32_t reference);
    VulkanPipelineBuilder& cullMode(VkCullModeFlags mode);
    VulkanPipelineBuilder& frontFace(VkFrontFace mode);
    VulkanPipelineBuilder& polygonMode(VkPolygonMode mode);
    VulkanPipelineBuilder& vertexInputState(const VkPipelineVertexInputStateCreateInfo& state);
    VulkanPipelineBuilder& colorAttachments(const VkPipelineColorBlendAttachmentState* states, const VkFormat* formats,
                                            uint32_t numColorAttachments);
    VulkanPipelineBuilder& depthAttachmentFormat(VkFormat format);
    VulkanPipelineBuilder& stencilAttachmentFormat(VkFormat format);

    VkResult build(Device* pDevice, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout,
                   VkPipeline* outPipeline) noexcept;

    static uint32_t getNumPipelinesCreated() { return numPipelinesCreated_; }

private:
    enum
    {
        APH_MAX_DYNAMIC_STATES = 128
    };
    uint32_t       numDynamicStates_                      = 0;
    VkDynamicState dynamicStates_[APH_MAX_DYNAMIC_STATES] = {};

    SmallVector<VkPipelineShaderStageCreateInfo> shaderStages_ = {};

    VkPipelineVertexInputStateCreateInfo   vertexInputState_;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_;
    VkPipelineRasterizationStateCreateInfo rasterizationState_;
    VkPipelineMultisampleStateCreateInfo   multisampleState_;
    VkPipelineDepthStencilStateCreateInfo  depthStencilState_;

    SmallVector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates_ = {};
    SmallVector<VkFormat>                            colorAttachmentFormats_     = {};

    VkFormat depthAttachmentFormat_   = VK_FORMAT_UNDEFINED;
    VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

    static uint32_t numPipelinesCreated_;
};

class Pipeline : public ResourceHandle<VkPipeline>
{
    friend class ObjectPool<Pipeline>;

public:
    DescriptorSet* acquireSet(uint32_t idx) const;

    ShaderProgram*      getProgram() const { return m_pProgram; }
    VkPipelineBindPoint getBindPoint() const { return m_bindPoint; }

protected:
    Pipeline(Device* pDevice, const RenderPipelineState& rps, HandleType handle, ShaderProgram* pProgram);
    Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, HandleType handle, ShaderProgram* pProgram);

    Device*             m_pDevice   = {};
    ShaderProgram*      m_pProgram  = {};
    VkPipelineBindPoint m_bindPoint = {};
    VkPipelineCache     m_cache     = {};
    RenderPipelineState m_rps       = {};
};

class PipelineAllocator
{
    struct HashGraphicsPipeline
    {
        std::size_t operator()(const ComputePipelineCreateInfo& info) const noexcept
        {
            std::size_t seed = 0;
            aph::utils::hashCombine(seed, info.pCompute);
            aph::utils::hashCombine(seed, info.pSamplerBank);
            return seed;
        }

        std::size_t operator()(const GraphicsPipelineCreateInfo& info) const noexcept
        {
            std::size_t seed = 0;
            aph::utils::hashCombine(seed, info.dynamicState.depthBiasEnable);
            aph::utils::hashCombine(seed, info.topology);

            {
                for(const auto& attr : info.vertexInput.attributes)
                {
                    aph::utils::hashCombine(seed, attr.binding);
                    aph::utils::hashCombine(seed, attr.format);
                    aph::utils::hashCombine(seed, attr.location);
                    aph::utils::hashCombine(seed, attr.offset);
                }
                for(const auto& binding : info.vertexInput.bindings)
                {
                    aph::utils::hashCombine(seed, binding.stride);
                }
            }

            aph::utils::hashCombine(seed, info.pProgram);

            for(auto color : info.color)
            {
                aph::utils::hashCombine(seed, color.format);
                aph::utils::hashCombine(seed, color.blendEnabled);
                aph::utils::hashCombine(seed, color.rgbBlendOp);
                aph::utils::hashCombine(seed, color.alphaBlendOp);
                aph::utils::hashCombine(seed, color.srcRGBBlendFactor);
                aph::utils::hashCombine(seed, color.srcAlphaBlendFactor);
                aph::utils::hashCombine(seed, color.dstRGBBlendFactor);
                aph::utils::hashCombine(seed, color.dstAlphaBlendFactor);
            }

            aph::utils::hashCombine(seed, info.depthFormat);
            aph::utils::hashCombine(seed, info.stencilFormat);

            aph::utils::hashCombine(seed, info.cullMode);
            aph::utils::hashCombine(seed, info.frontFaceWinding);
            aph::utils::hashCombine(seed, info.polygonMode);

            {
                auto& stencilState = info.backFaceStencil;
                aph::utils::hashCombine(seed, stencilState.stencilFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthStencilPassOp);
                aph::utils::hashCombine(seed, stencilState.stencilCompareOp);
                aph::utils::hashCombine(seed, stencilState.readMask);
                aph::utils::hashCombine(seed, stencilState.writeMask);
            }

            {
                auto& stencilState = info.frontFaceStencil;
                aph::utils::hashCombine(seed, stencilState.stencilFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthFailureOp);
                aph::utils::hashCombine(seed, stencilState.depthStencilPassOp);
                aph::utils::hashCombine(seed, stencilState.stencilCompareOp);
                aph::utils::hashCombine(seed, stencilState.readMask);
                aph::utils::hashCombine(seed, stencilState.writeMask);
            }

            aph::utils::hashCombine(seed, info.samplesCount);

            return seed;
        }
    };

public:
    PipelineAllocator(Device* pDevice) : m_pDevice(pDevice) {}
    ~PipelineAllocator();

    void clear();

    Pipeline* getPipeline(const GraphicsPipelineCreateInfo& createInfo);
    Pipeline* getPipeline(const ComputePipelineCreateInfo& createInfo);

private:
    Device*                                                              m_pDevice = {};
    HashMap<GraphicsPipelineCreateInfo, Pipeline*, HashGraphicsPipeline> m_graphicsPipelineMap;
    HashMap<ComputePipelineCreateInfo, Pipeline*, HashGraphicsPipeline>  m_computePipelineMap;
    ThreadSafeObjectPool<Pipeline>                                       m_pool;
};

}  // namespace aph::vk

#endif  // PIPELINE_H_
