#include "pipeline.h"
#include "api/vulkan/vkInit.h"
#include "device.h"
#include "shader.h"

namespace aph::vk
{

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
                   VkPipeline* outPipeline, PipelineType type) noexcept;

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

uint32_t VulkanPipelineBuilder::numPipelinesCreated_ = 0;

VulkanPipelineBuilder::VulkanPipelineBuilder() :
    vertexInputState_(VkPipelineVertexInputStateCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr,
    }),
    inputAssembly_({
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags                  = 0,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    }),
    rasterizationState_({
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .flags                   = 0,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_NONE,
        .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = 0.0f,
        .lineWidth               = 1.0f,
    }),
    multisampleState_({
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 1.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    }),
    depthStencilState_({
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = VK_FALSE,
        .depthWriteEnable      = VK_FALSE,
        .depthCompareOp        = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = VK_FALSE,
        .front =
            {
                .failOp      = VK_STENCIL_OP_KEEP,
                .passOp      = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp   = VK_COMPARE_OP_NEVER,
                .compareMask = 0,
                .writeMask   = 0,
                .reference   = 0,
            },
        .back =
            {
                .failOp      = VK_STENCIL_OP_KEEP,
                .passOp      = VK_STENCIL_OP_KEEP,
                .depthFailOp = VK_STENCIL_OP_KEEP,
                .compareOp   = VK_COMPARE_OP_NEVER,
                .compareMask = 0,
                .writeMask   = 0,
                .reference   = 0,
            },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    })
{
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthBiasEnable(bool enable)
{
    rasterizationState_.depthBiasEnable = enable ? VK_TRUE : VK_FALSE;
    return *this;
}

#if defined(__APPLE__)
VulkanPipelineBuilder& VulkanPipelineBuilder::depthWriteEnable(bool enable)
{
    depthStencilState_.depthWriteEnable = enable ? VK_TRUE : VK_FALSE;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthCompareOp(VkCompareOp compareOp)
{
    depthStencilState_.depthTestEnable = compareOp != VK_COMPARE_OP_ALWAYS;
    depthStencilState_.depthCompareOp  = compareOp;
    return *this;
}
#endif

VulkanPipelineBuilder& VulkanPipelineBuilder::dynamicState(VkDynamicState state)
{
    APH_ASSERT(numDynamicStates_ < APH_MAX_DYNAMIC_STATES);
    dynamicStates_[numDynamicStates_++] = state;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::primitiveTopology(VkPrimitiveTopology topology)
{
    inputAssembly_.topology = topology;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::rasterizationSamples(VkSampleCountFlagBits samples)
{
    multisampleState_.rasterizationSamples = samples;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::cullMode(VkCullModeFlags mode)
{
    rasterizationState_.cullMode = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::frontFace(VkFrontFace mode)
{
    rasterizationState_.frontFace = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::polygonMode(VkPolygonMode mode)
{
    rasterizationState_.polygonMode = mode;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::vertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
    vertexInputState_ = state;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::colorAttachments(const VkPipelineColorBlendAttachmentState* states,
                                                               const VkFormat* formats, uint32_t numColorAttachments)
{
    APH_ASSERT(states);
    APH_ASSERT(formats);
    colorBlendAttachmentStates_.resize(numColorAttachments);
    colorAttachmentFormats_.resize(numColorAttachments);
    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        colorBlendAttachmentStates_[i] = states[i];
        colorAttachmentFormats_[i]     = formats[i];
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::depthAttachmentFormat(VkFormat format)
{
    depthAttachmentFormat_ = format;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilAttachmentFormat(VkFormat format)
{
    stencilAttachmentFormat_ = format;
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage)
{
    if(stage.module != VK_NULL_HANDLE)
    {
        shaderStages_.push_back(stage);
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::shaderStage(const SmallVector<VkPipelineShaderStageCreateInfo>& stages)
{
    for(auto stage : stages)
    {
        shaderStage(stage);
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp,
                                                              VkStencilOp passOp, VkStencilOp depthFailOp,
                                                              VkCompareOp compareOp)
{
    if(faceMask & VK_STENCIL_FACE_FRONT_BIT)
    {
        VkStencilOpState& s = depthStencilState_.front;
        s.failOp            = failOp;
        s.passOp            = passOp;
        s.depthFailOp       = depthFailOp;
        s.compareOp         = compareOp;
    }

    if(faceMask & VK_STENCIL_FACE_BACK_BIT)
    {
        VkStencilOpState& s = depthStencilState_.back;
        s.failOp            = failOp;
        s.passOp            = passOp;
        s.depthFailOp       = depthFailOp;
        s.compareOp         = compareOp;
    }
    return *this;
}

VulkanPipelineBuilder& VulkanPipelineBuilder::stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask,
                                                           uint32_t writeMask, uint32_t reference)
{
    if(faceMask & VK_STENCIL_FACE_FRONT_BIT)
    {
        VkStencilOpState& s = depthStencilState_.front;
        s.compareMask       = compareMask;
        s.writeMask         = writeMask;
        s.reference         = reference;
    }

    if(faceMask & VK_STENCIL_FACE_BACK_BIT)
    {
        VkStencilOpState& s = depthStencilState_.back;
        s.compareMask       = compareMask;
        s.writeMask         = writeMask;
        s.reference         = reference;
    }
    return *this;
}

VkResult VulkanPipelineBuilder::build(Device* pDevice, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout,
                                      VkPipeline* outPipeline, PipelineType type) noexcept
{
    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = numDynamicStates_,
        .pDynamicStates    = dynamicStates_,
    };
    // viewport and scissor can be NULL if the viewport state is dynamic
    // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineViewportStateCreateInfo.html
    const VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = nullptr,
        .scissorCount  = 1,
        .pScissors     = nullptr,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<uint32_t>(colorBlendAttachmentStates_.size()),
        .pAttachments    = colorBlendAttachmentStates_.data(),
    };
    const VkPipelineRenderingCreateInfo renderingInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .pNext                   = nullptr,
        .colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentFormats_.size()),
        .pColorAttachmentFormats = colorAttachmentFormats_.data(),
        .depthAttachmentFormat   = depthAttachmentFormat_,
        .stencilAttachmentFormat = stencilAttachmentFormat_,
    };

    bool isGeometryPipeline = type == PipelineType::Geometry;
    const VkGraphicsPipelineCreateInfo ci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &renderingInfo,
        .flags               = 0,
        .stageCount          = static_cast<uint32_t>(shaderStages_.size()),
        .pStages             = shaderStages_.data(),
        .pVertexInputState   = isGeometryPipeline ? &vertexInputState_ : nullptr,
        .pInputAssemblyState = isGeometryPipeline ? &inputAssembly_ : nullptr,
        .pTessellationState  = nullptr,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizationState_,
        .pMultisampleState   = &multisampleState_,
        .pDepthStencilState  = &depthStencilState_,
        .pColorBlendState    = &colorBlendState,
        .pDynamicState       = &dynamicState,
        .layout              = pipelineLayout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };

    const auto result = pDevice->getDeviceTable()->vkCreateGraphicsPipelines(pDevice->getHandle(), pipelineCache, 1,
                                                                             &ci, vkAllocator(), outPipeline);

    if(result != VK_SUCCESS)
    {
        return result;
    }

    numPipelinesCreated_++;

    return VK_SUCCESS;
}

Pipeline::Pipeline(Device* pDevice, const ComputePipelineCreateInfo& createInfo, HandleType handle,
                   ShaderProgram* pProgram) :
    ResourceHandle(handle),
    m_pDevice(pDevice),
    m_pProgram(pProgram),
    m_type(PipelineType::Compute)
{
    APH_ASSERT(pProgram);
}

Pipeline::Pipeline(Device* pDevice, const GraphicsPipelineCreateInfo& createInfo, HandleType handle,
                   ShaderProgram* pProgram) :
    ResourceHandle(handle),
    m_pDevice(pDevice),
    m_pProgram(pProgram),
    m_type(createInfo.type)
{
    APH_ASSERT(pProgram);
}

DescriptorSet* Pipeline::acquireSet(uint32_t idx) const
{
    return m_pProgram->getSetLayout(idx)->allocateSet();
}

Pipeline* PipelineAllocator::getPipeline(const ComputePipelineCreateInfo& createInfo)
{
    std::lock_guard<std::mutex> holder{m_computeAcquireLock};
    if(!m_computePipelineMap.contains(createInfo))
    {
        m_computePipelineMap[createInfo] = create(createInfo);
    }
    return m_computePipelineMap[createInfo];
}
Pipeline* PipelineAllocator::getPipeline(const GraphicsPipelineCreateInfo& createInfo)
{
    std::lock_guard<std::mutex> holder{m_graphicsAcquireLock};
    if(!m_graphicsPipelineMap.contains(createInfo))
    {
        m_graphicsPipelineMap[createInfo] = create(createInfo);
    }
    return m_graphicsPipelineMap[createInfo];
}

PipelineAllocator::~PipelineAllocator() = default;

void PipelineAllocator::clear()
{
    auto& table = *m_pDevice->getDeviceTable();
    for(auto& [_, pPipeline] : m_graphicsPipelineMap)
    {
        table.vkDestroyPipeline(m_pDevice->getHandle(), pPipeline->getHandle(), vkAllocator());
        m_pool.free(pPipeline);
    }
    for(auto& [_, pPipeline] : m_computePipelineMap)
    {
        table.vkDestroyPipeline(m_pDevice->getHandle(), pPipeline->getHandle(), vkAllocator());
        m_pool.free(pPipeline);
    }
}

Pipeline* PipelineAllocator::create(const GraphicsPipelineCreateInfo& createInfo)
{
    Pipeline*                                    pPipeline = {};
    SmallVector<VkPipelineShaderStageCreateInfo> shaderStages;

    auto& pProgram = createInfo.pProgram;
    APH_ASSERT(pProgram);

    // Not all attachments are valid. We need to create color blend attachments only for active attachments
    const uint32_t numColorAttachments = createInfo.color.size();

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates{numColorAttachments};
    std::vector<VkFormat>                            colorAttachmentFormats{numColorAttachments};
    for(uint32_t i = 0; i != numColorAttachments; i++)
    {
        const auto& attachment = createInfo.color[i];
        APH_ASSERT(attachment.format != Format::Undefined);
        colorAttachmentFormats[i] = utils::VkCast(attachment.format);
        if(!attachment.blendEnabled)
        {
            colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable         = VK_FALSE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                .colorBlendOp        = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp        = VK_BLEND_OP_ADD,
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
        }
        else
        {
            colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{
                .blendEnable         = VK_TRUE,
                .srcColorBlendFactor = utils::VkCast(attachment.srcRGBBlendFactor),
                .dstColorBlendFactor = utils::VkCast(attachment.dstRGBBlendFactor),
                .colorBlendOp        = utils::VkCast(attachment.rgbBlendOp),
                .srcAlphaBlendFactor = utils::VkCast(attachment.srcAlphaBlendFactor),
                .dstAlphaBlendFactor = utils::VkCast(attachment.dstAlphaBlendFactor),
                .alphaBlendOp        = utils::VkCast(attachment.alphaBlendOp),
                .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                  VK_COLOR_COMPONENT_A_BIT,
            };
        }
    }

    VkPipeline handle;

    auto builder =
        VulkanPipelineBuilder()
            // from Vulkan 1.0
            .dynamicState(VK_DYNAMIC_STATE_VIEWPORT)
            .dynamicState(VK_DYNAMIC_STATE_SCISSOR)
            .dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS)
            .dynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS)
            // from Vulkan 1.3
            .dynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE)
            .dynamicState(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE)
            .dynamicState(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP)
            .depthBiasEnable(createInfo.dynamicState.depthBiasEnable)
            .rasterizationSamples(aph::vk::utils::getSampleCountFlags(createInfo.samplesCount))
            .polygonMode(utils::VkCast(createInfo.polygonMode))
            .stencilStateOps(VK_STENCIL_FACE_FRONT_BIT, utils::VkCast(createInfo.frontFaceStencil.stencilFailureOp),
                             utils::VkCast(createInfo.frontFaceStencil.depthStencilPassOp),
                             utils::VkCast(createInfo.frontFaceStencil.depthFailureOp),
                             utils::VkCast(createInfo.frontFaceStencil.stencilCompareOp))
            .stencilStateOps(VK_STENCIL_FACE_BACK_BIT, utils::VkCast(createInfo.backFaceStencil.stencilFailureOp),
                             utils::VkCast(createInfo.backFaceStencil.depthStencilPassOp),
                             utils::VkCast(createInfo.backFaceStencil.depthFailureOp),
                             utils::VkCast(createInfo.backFaceStencil.stencilCompareOp))
            .stencilMasks(VK_STENCIL_FACE_FRONT_BIT, 0xFF, createInfo.frontFaceStencil.writeMask,
                          createInfo.frontFaceStencil.readMask)
            .stencilMasks(VK_STENCIL_FACE_BACK_BIT, 0xFF, createInfo.backFaceStencil.writeMask,
                          createInfo.backFaceStencil.readMask)
            .cullMode(utils::VkCast(createInfo.cullMode))
            .frontFace(utils::VkCast(createInfo.frontFaceWinding))
            .colorAttachments(colorBlendAttachmentStates.data(), colorAttachmentFormats.data(), numColorAttachments)
            .depthAttachmentFormat(utils::VkCast(createInfo.depthFormat))
            .stencilAttachmentFormat(utils::VkCast(createInfo.stencilFormat));

    std::vector<VkVertexInputBindingDescription>   vkBindings;
    std::vector<VkVertexInputAttributeDescription> vkAttributes;
    switch(createInfo.type)
    {
    case PipelineType::Geometry:
    {
        shaderStages.push_back(init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                                                   pProgram->getShader(ShaderStage::VS)->getHandle()));
        const VertexInput& vstate = createInfo.vertexInput;
        vkAttributes.resize(vstate.attributes.size());
        SmallVector<bool> bufferAlreadyBound(vstate.bindings.size());

        for(uint32_t i = 0; i != vkAttributes.size(); i++)
        {
            const auto& attr = vstate.attributes[i];

            vkAttributes[i] = {.location = attr.location,
                               .binding  = attr.binding,
                               .format   = utils::VkCast(attr.format),
                               .offset   = (uint32_t)attr.offset};

            if(!bufferAlreadyBound[attr.binding])
            {
                bufferAlreadyBound[attr.binding] = true;
                vkBindings.push_back({.binding   = attr.binding,
                                      .stride    = vstate.bindings[attr.binding].stride,
                                      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX});
            }
        }
        const VkPipelineVertexInputStateCreateInfo ciVertexInputState = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<uint32_t>(vkBindings.size()),
            .pVertexBindingDescriptions      = !vkBindings.empty() ? vkBindings.data() : nullptr,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size()),
            .pVertexAttributeDescriptions    = !vkAttributes.empty() ? vkAttributes.data() : nullptr,
        };
        builder.primitiveTopology(utils::VkCast(createInfo.topology));
        builder.vertexInputState(ciVertexInputState);
    }
    break;
    case PipelineType::Mesh:
    {
        shaderStages.push_back(init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_MESH_BIT_EXT,
                                                                   pProgram->getShader(ShaderStage::MS)->getHandle()));
        if(auto taskShader = pProgram->getShader(ShaderStage::TS); taskShader != nullptr)
        {
            shaderStages.push_back(
                init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_TASK_BIT_EXT, taskShader->getHandle()));
        }
    }
    break;
    case PipelineType::Compute:
    case PipelineType::RayTracing:
        APH_ASSERT(false);
        return nullptr;
    }

    shaderStages.push_back(init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                                               pProgram->getShader(ShaderStage::FS)->getHandle()));
    builder.shaderStage(shaderStages);
    builder.build(m_pDevice, VK_NULL_HANDLE, pProgram->getPipelineLayout(), &handle, createInfo.type);

    pPipeline = m_pool.allocate(m_pDevice, createInfo, handle, pProgram);
    return pPipeline;
}
Pipeline* PipelineAllocator::create(const ComputePipelineCreateInfo& createInfo)
{
    APH_ASSERT(createInfo.pCompute);
    ShaderProgram*              program = createInfo.pCompute;
    VkComputePipelineCreateInfo ci      = init::computePipelineCreateInfo(program->getPipelineLayout());
    ci.stage                            = init::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT,
                                                                              program->getShader(ShaderStage::CS)->getHandle());
    VkPipeline handle                   = VK_NULL_HANDLE;
    _VR(m_pDevice->getDeviceTable()->vkCreateComputePipelines(m_pDevice->getHandle(), VK_NULL_HANDLE, 1, &ci,
                                                              vkAllocator(), &handle));
    return m_pool.allocate(m_pDevice, createInfo, handle, program);
}
}  // namespace aph::vk
