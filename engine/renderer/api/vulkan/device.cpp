#include "device.h"
#include "buffer.h"
#include "commandBuffer.h"
#include "shader.h"
#include "pipeline.h"
#include "commandPool.h"
#include "framebuffer.h"
#include "image.h"
#include "imageView.h"
#include "renderpass.h"
#include "swapChain.h"
#include "vkInit.hpp"
#include "vkUtils.h"

namespace vkl {
VkResult VulkanDevice::Create(VulkanPhysicalDevice *pPhysicalDevice, const DeviceCreateInfo *pCreateInfo, VulkanDevice **ppDevice) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    const float                          defaultQueuePriority(0.0f);

    if (pCreateInfo->requestQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS);
        queueInfo.queueCount       = 1;
        queueInfo.pQueuePriorities = &defaultQueuePriority;

        queueCreateInfos.push_back(queueInfo);
    }
    if (pCreateInfo->requestQueueTypes & VK_QUEUE_COMPUTE_BIT) {
        if (pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_COMPUTE) != pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS)) {
            // If compute family index differs, we need an additional queue create info for the compute queue
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_COMPUTE);
            queueInfo.queueCount       = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
    }
    if (pCreateInfo->requestQueueTypes & VK_QUEUE_TRANSFER_BIT) {
        if ((pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_TRANSFER) != pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS)) &&
            (pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_TRANSFER) != pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_COMPUTE))) {
            // If transfer family index differs, we need an additional queue create info for the transfer queue
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_TRANSFER);
            queueInfo.queueCount       = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            queueCreateInfos.push_back(queueInfo);
        }
    }

    // Create the Vulkan device.
    VkDeviceCreateInfo deviceCreateInfo      = {};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext                   = pCreateInfo->pNext;
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pEnabledFeatures        = &pPhysicalDevice->getDeviceFeatures();
    deviceCreateInfo.enabledLayerCount       = pCreateInfo->enabledLayerCount;
    deviceCreateInfo.ppEnabledLayerNames     = pCreateInfo->ppEnabledLayerNames;
    deviceCreateInfo.enabledExtensionCount   = pCreateInfo->enabledExtensionCount;
    deviceCreateInfo.ppEnabledExtensionNames = pCreateInfo->ppEnabledExtensionNames;

    VkDevice handle = VK_NULL_HANDLE;
    auto     result = vkCreateDevice(pPhysicalDevice->getHandle(), &deviceCreateInfo, nullptr, &handle);
    if (result != VK_SUCCESS)
        return result;

    // Initialize Device class.
    auto *device = new VulkanDevice;
    memcpy(&device->_createInfo, pCreateInfo, sizeof(DeviceCreateInfo));
    device->_physicalDevice = pPhysicalDevice;
    device->_handle         = handle;

    VK_CHECK_RESULT(device->createCommandPool(&device->_drawCommandPool, pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS)));
    VK_CHECK_RESULT(device->createCommandPool(&device->_transferCommandPool, pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_TRANSFER)));
    VK_CHECK_RESULT(device->createCommandPool(&device->_computeCommandPool, pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_COMPUTE)));

    for (auto i = 0; i < queueCreateInfos.size(); i++) {
        for (auto j = 0; j < queueCreateInfos[i].queueCount; j++) {
            VkQueue queue = VK_NULL_HANDLE;
            vkGetDeviceQueue(device->getHandle(), i, j, &queue);
            if (queue) {
                QueueFamily *qf = nullptr;
                if (queueCreateInfos[i].queueFamilyIndex == pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_GRAPHICS)) {
                    device->_queues[QUEUE_TYPE_GRAPHICS].push_back(queue);
                }
                if (queueCreateInfos[i].queueFamilyIndex == pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_TRANSFER)) {
                    device->_queues[QUEUE_TYPE_TRANSFER].push_back(queue);
                }
                if (queueCreateInfos[i].queueFamilyIndex == pPhysicalDevice->getQueueFamilyIndices(QUEUE_TYPE_COMPUTE)) {
                    device->_queues[QUEUE_TYPE_COMPUTE].push_back(queue);
                }
            }
        }
    }

    // Copy address of object instance.
    *ppDevice = device;

    // Return success.
    return VK_SUCCESS;
}

void VulkanDevice::Destroy(VulkanDevice *pDevice) {
    if (pDevice->_drawCommandPool) {
        pDevice->destroyCommandPool(pDevice->_drawCommandPool);
    }
    if (pDevice->_transferCommandPool) {
        pDevice->destroyCommandPool(pDevice->_transferCommandPool);
    }
    if (pDevice->_computeCommandPool) {
        pDevice->destroyCommandPool(pDevice->_computeCommandPool);
    }

    if (pDevice->_handle) {
        vkDestroyDevice(pDevice->_handle, nullptr);
    }
    delete pDevice;
}

VkResult VulkanDevice::createCommandPool(VulkanCommandPool **ppPool, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags) {
    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex        = queueFamilyIndex;
    cmdPoolInfo.flags                   = createFlags;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    auto          result  = vkCreateCommandPool(_handle, &cmdPoolInfo, nullptr, &cmdPool);
    if (result != VK_SUCCESS) {
        return result;
    }

    *ppPool = VulkanCommandPool::Create(this, queueFamilyIndex, cmdPool);
    return VK_SUCCESS;
}

VkFormat VulkanDevice::getDepthFormat() const {
    return _physicalDevice->findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                                VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkResult VulkanDevice::createImageView(ImageViewCreateInfo *pCreateInfo, VulkanImageView **ppImageView, VulkanImage *pImage) {
    // Create a new Vulkan image view.
    VkImageViewCreateInfo createInfo = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.image                 = pImage->getHandle();
    createInfo.viewType              = static_cast<VkImageViewType>(pCreateInfo->viewType);
    createInfo.format                = static_cast<VkFormat>(pCreateInfo->format);
    memcpy(&createInfo.components, &pCreateInfo->components, sizeof(VkComponentMapping));
    createInfo.subresourceRange.aspectMask     = vkl::utils::getImageAspectFlags(static_cast<VkFormat>(createInfo.format));
    createInfo.subresourceRange.baseMipLevel   = pCreateInfo->subresourceRange.baseMipLevel;
    createInfo.subresourceRange.levelCount     = pCreateInfo->subresourceRange.levelCount;
    createInfo.subresourceRange.baseArrayLayer = pCreateInfo->subresourceRange.baseArrayLayer;
    createInfo.subresourceRange.layerCount     = pCreateInfo->subresourceRange.layerCount;
    VkImageView handle                         = VK_NULL_HANDLE;
    auto        result                         = vkCreateImageView(pImage->getDevice()->getHandle(), &createInfo, nullptr, &handle);
    if (result != VK_SUCCESS)
        return result;

    *ppImageView = VulkanImageView::createFromHandle(pCreateInfo, pImage, handle);

    return VK_SUCCESS;
}

void VulkanDevice::endSingleTimeCommands(VulkanCommandBuffer *commandBuffer) {
    auto flags = commandBuffer->getQueueFamilyTypes();
    commandBuffer->end();

    VkSubmitInfo submitInfo = vkl::init::submitInfo(&commandBuffer->getHandle());

    vkQueueSubmit(getQueueByFlags(flags), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(getQueueByFlags(flags));

    freeCommandBuffers(1, &commandBuffer);
}

VulkanCommandBuffer *VulkanDevice::beginSingleTimeCommands(QueueFamilyType flags) {
    VulkanCommandBuffer *instance;
    allocateCommandBuffers(1, &instance, flags);
    instance->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return instance;
}

VkResult VulkanDevice::createBuffer(BufferCreateInfo *pCreateInfo, VulkanBuffer **ppBuffer, void *data) {
    VkBuffer       buffer;
    VkDeviceMemory memory;
    // create buffer
    VkBufferCreateInfo bufferInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = pCreateInfo->size,
        .usage       = pCreateInfo->usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VK_CHECK_RESULT(vkCreateBuffer(_handle, &bufferInfo, nullptr, &buffer));

    // create memory
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(_handle, buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = _physicalDevice->findMemoryType(memRequirements.memoryTypeBits, pCreateInfo->property),
    };
    VK_CHECK_RESULT(vkAllocateMemory(_handle, &allocInfo, nullptr, &memory));

    *ppBuffer = VulkanBuffer::CreateFromHandle(this, pCreateInfo, buffer, memory);

    // bind buffer and memory
    VkResult result = (*ppBuffer)->bind();
    VK_CHECK_RESULT(result);

    if (data) {
        (*ppBuffer)->map();
        (*ppBuffer)->copyTo(data, (*ppBuffer)->getSize());
        (*ppBuffer)->unmap();
    }

    return result;
}

VkResult VulkanDevice::createImage(ImageCreateInfo *pCreateInfo, VulkanImage **ppImage) {
    VkImage        image;
    VkDeviceMemory memory;

    VkImageCreateInfo imageCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = static_cast<VkFormat>(pCreateInfo->format),
        .mipLevels     = pCreateInfo->mipLevels,
        .arrayLayers   = pCreateInfo->layerCount,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = static_cast<VkImageTiling>(pCreateInfo->tiling),
        .usage         = pCreateInfo->usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    imageCreateInfo.extent.width  = pCreateInfo->extent.width;
    imageCreateInfo.extent.height = pCreateInfo->extent.height;
    imageCreateInfo.extent.depth  = pCreateInfo->extent.depth;

    if (vkCreateImage(_handle, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(_handle, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = memRequirements.size,
        .memoryTypeIndex = _physicalDevice->findMemoryType(memRequirements.memoryTypeBits, pCreateInfo->property),
    };

    if (vkAllocateMemory(_handle, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    *ppImage = VulkanImage::CreateFromHandle(this, pCreateInfo, image, memory);

    if ((*ppImage)->getMemory() != VK_NULL_HANDLE) {
        auto result = (*ppImage)->bind();
        return result;
    }

    return VK_SUCCESS;
}

VulkanPhysicalDevice *VulkanDevice::getPhysicalDevice() const {
    return _physicalDevice;
}

VkResult VulkanDevice::createRenderPass(RenderPassCreateInfo                       *createInfo,
                                        VulkanRenderPass                          **ppRenderPass,
                                        const std::vector<VkAttachmentDescription> &colorAttachments,
                                        const VkAttachmentDescription              &depthAttachment) {

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference>   colorAttachmentRefs;

    for (uint32_t idx = 0; idx < colorAttachments.size(); idx++) {
        attachments.push_back(colorAttachments[idx]);
        VkAttachmentReference ref{};
        ref.attachment = idx;
        ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(ref);
    }

    attachments.push_back(depthAttachment);
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = static_cast<uint32_t>(colorAttachments.size());
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount    = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpassDescription.pColorAttachments       = colorAttachmentRefs.data();
    subpassDescription.pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass      = 0;
    dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask   = VK_ACCESS_NONE_KHR;
    dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass      = 0;
    dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask   = VK_ACCESS_NONE_KHR;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo{
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments    = attachments.data(),
        .subpassCount    = 1,
        .pSubpasses      = &subpassDescription,
        .dependencyCount = dependencies.size(),
        .pDependencies   = dependencies.data(),
    };

    VkRenderPass renderpass;
    auto         result = vkCreateRenderPass(_handle, &renderPassInfo, nullptr, &renderpass);

    if (result != VK_SUCCESS) {
        return result;
    }

    *ppRenderPass = new VulkanRenderPass(renderpass, colorAttachmentRefs.size());

    return VK_SUCCESS;
}

VkResult VulkanDevice::createFramebuffers(FramebufferCreateInfo *pCreateInfo,
                                          VulkanFramebuffer    **ppFramebuffer,
                                          uint32_t               attachmentCount,
                                          VulkanImageView      **pAttachments) {
    return VulkanFramebuffer::Create(this, pCreateInfo, ppFramebuffer, attachmentCount, pAttachments);
}

void VulkanDevice::destroyBuffer(VulkanBuffer *pBuffer) {
    if (pBuffer->getMemory() != VK_NULL_HANDLE) {
        vkFreeMemory(_handle, pBuffer->getMemory(), nullptr);
    }
    vkDestroyBuffer(_handle, pBuffer->getHandle(), nullptr);
    delete pBuffer;
}
void VulkanDevice::destroyImage(VulkanImage *pImage) {
    if (pImage->getMemory() != VK_NULL_HANDLE) {
        vkFreeMemory(_handle, pImage->getMemory(), nullptr);
    }
    vkDestroyImage(_handle, pImage->getHandle(), nullptr);
    delete pImage;
}
void VulkanDevice::destroyImageView(VulkanImageView *pImageView) {
    vkDestroyImageView(_handle, pImageView->getHandle(), nullptr);
    delete pImageView;
}
void VulkanDevice::destoryRenderPass(VulkanRenderPass *pRenderpass) {
    vkDestroyRenderPass(_handle, pRenderpass->getHandle(), nullptr);
    delete pRenderpass;
}
void VulkanDevice::destroyFramebuffers(VulkanFramebuffer *pFramebuffer) {
    delete pFramebuffer;
}
VkResult VulkanDevice::createSwapchain(VkSurfaceKHR surface, VulkanSwapChain **ppSwapchain, WindowData *data) {
    VulkanSwapChain *instance = new VulkanSwapChain;
    instance->create(this, surface, data);

    // get present queue family
    {
        VkBool32                presentSupport = false;
        std::optional<uint32_t> presentQueueFamilyIndices;
        uint32_t                i = 0;

        for (const auto &queueFamily : _physicalDevice->getQueueFamilyProperties()) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_physicalDevice->getHandle(), i, surface, &presentSupport);
            if (presentSupport) {
                presentQueueFamilyIndices = i;
                break;
            }
            i++;
        }
        assert(presentQueueFamilyIndices.has_value());
        _queues[QUEUE_TYPE_PRESENT] = _queues[QUEUE_TYPE_GRAPHICS];
    }

    *ppSwapchain = instance;

    // TODO
    return VK_SUCCESS;
}
void VulkanDevice::destroySwapchain(VulkanSwapChain *pSwapchain) {
    vkDestroySwapchainKHR(getHandle(), pSwapchain->getHandle(), nullptr);
    delete pSwapchain;
}
VkQueue VulkanDevice::getQueueByFlags(QueueFamilyType queueFlags, uint32_t queueIndex) {
    return _queues[queueFlags][queueIndex];
}
void VulkanDevice::waitIdle() {
    vkDeviceWaitIdle(getHandle());
}

VulkanCommandPool *VulkanDevice::getCommandPoolWithQueue(QueueFamilyType type) {
    switch (type) {
    case QUEUE_TYPE_COMPUTE:
        return _computeCommandPool;
    case QUEUE_TYPE_GRAPHICS:
    case QUEUE_TYPE_PRESENT:
        return _drawCommandPool;
    case QUEUE_TYPE_TRANSFER:
        return _transferCommandPool;
    default:
        return _drawCommandPool;
    }
}

void VulkanDevice::destroyCommandPool(VulkanCommandPool *pPool) {
    vkDestroyCommandPool(getHandle(), pPool->getHandle(), nullptr);
    delete pPool;
}

VkResult VulkanDevice::allocateCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers, QueueFamilyType flags) {
    auto* pool = getCommandPoolWithQueue(flags);

    std::vector<VkCommandBuffer> handles(commandBufferCount);
    auto                         result = pool->allocateCommandBuffers(commandBufferCount, handles.data());
    if (result != VK_SUCCESS) {
        return result;
    }

    for (auto i = 0; i < commandBufferCount; i++) {
        ppCommandBuffers[i] = new VulkanCommandBuffer(pool, handles[i], flags);
    }
    return VK_SUCCESS;
}

void VulkanDevice::freeCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer **ppCommandBuffers) {
    // Destroy all of the command buffers.
    for (auto i = 0U; i < commandBufferCount; ++i) {
        delete ppCommandBuffers[i];
    }
}
VkResult VulkanDevice::createGraphicsPipeline(const PipelineCreateInfo *pCreateInfo,
                                              ShaderEffect             *pEffect,
                                              VulkanRenderPass         *pRenderPass,
                                              VulkanPipeline ** ppPipeline) {
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext                             = nullptr;

    viewportState.viewportCount = 1;
    viewportState.pViewports    = &pCreateInfo->_viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &pCreateInfo->_scissor;

    // setup dummy color blending. We aren't using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext                               = nullptr;

    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.logicOp         = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &pCreateInfo->_colorBlendAttachment;

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = nullptr;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (const auto [stage, sModule] : pEffect->getStages()) {
        shaderStages.push_back(vkl::init::pipelineShaderStageCreateInfo(stage, sModule->getHandle()));
    }
    pipelineInfo.stageCount          = shaderStages.size();
    pipelineInfo.pStages             = shaderStages.data();
    pipelineInfo.pVertexInputState   = &pCreateInfo->_vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &pCreateInfo->_inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pDynamicState       = &pCreateInfo->_dynamicState;
    pipelineInfo.pRasterizationState = &pCreateInfo->_rasterizer;
    pipelineInfo.pDepthStencilState  = &pCreateInfo->_depthStencil;
    pipelineInfo.pMultisampleState   = &pCreateInfo->_multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.layout              = pEffect->getPipelineLayout();
    pipelineInfo.renderPass          = pRenderPass->getHandle();
    pipelineInfo.subpass             = 0;
    pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

    VkPipeline handle;
    auto result = vkCreateGraphicsPipelines(getHandle(), pCreateInfo->_pipelineCache, 1, &pipelineInfo, nullptr, &handle);

    if(result != VK_SUCCESS){
        return result;
    }

    *ppPipeline = VulkanPipeline::CreateGraphicsPipeline(this, pCreateInfo, pEffect, pRenderPass, handle);

    return VK_SUCCESS;
}

void VulkanDevice::destroyPipeline(VulkanPipeline *pipeline) {
    vkDestroyPipeline(getHandle(), pipeline->getHandle(), nullptr);
    delete pipeline;
}
} // namespace vkl
