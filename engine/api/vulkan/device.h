#ifndef VULKAN_DEVICE_H_
#define VULKAN_DEVICE_H_

#include "buffer.h"
#include "commandBuffer.h"
#include "commandPool.h"
#include "descriptorPool.h"
#include "descriptorSetLayout.h"
#include "image.h"
#include "physicalDevice.h"
#include "pipeline.h"
#include "queue.h"
#include "shader.h"
#include "swapChain.h"
#include "syncPrimitivesPool.h"
#include "vkInit.h"
#include "vkUtils.h"

namespace aph
{

enum DeviceCreateFlagBits
{
    // TODO
};
using DeviceCreateFlags = uint32_t;

struct DeviceCreateInfo
{
    DeviceCreateFlags        flags;
    std::vector<const char*> enabledExtensions;
    VulkanPhysicalDevice*    pPhysicalDevice = nullptr;
};

class VulkanDevice : public ResourceHandle<VkDevice, DeviceCreateInfo>
{
private:
    VulkanDevice(const DeviceCreateInfo& createInfo, VulkanPhysicalDevice* pPhysicalDevice, VkDevice handle);

public:
    static VkResult Create(const DeviceCreateInfo& createInfo, VulkanDevice** ppDevice);

    static void Destroy(VulkanDevice* pDevice);

public:
    VkResult createCubeMap(const std::array<std::shared_ptr<ImageInfo>, 6>& images, VulkanImage** ppImage,
                           VulkanImageView** ppImageView);
    VkResult createDeviceLocalBuffer(const BufferCreateInfo& createInfo, VulkanBuffer** ppBuffer, const void* data);
    VkResult createDeviceLocalImage(const ImageCreateInfo& createInfo, VulkanImage** ppImage,
                                    const std::vector<uint8_t>& data);
    VkResult executeSingleCommands(QueueTypeFlags                                               type,
                                   const std::function<void(VulkanCommandBuffer* pCmdBuffer)>&& func);

public:
    VkResult createSampler(const VkSamplerCreateInfo& createInfo, VkSampler * pSampler);
    VkResult createBuffer(const BufferCreateInfo& createInfo, VulkanBuffer** ppBuffer, const void* data = nullptr,
                          bool persistmentMap = false);
    VkResult createImage(const ImageCreateInfo& createInfo, VulkanImage** ppImage);
    VkResult createImageView(const ImageViewCreateInfo& createInfo, VulkanImageView** ppImageView, VulkanImage* pImage);
    VkResult createSwapchain(const SwapChainCreateInfo& createInfo, VulkanSwapChain** ppSwapchain);
    VkResult createCommandPool(const CommandPoolCreateInfo& createInfo, VulkanCommandPool** ppPool);
    VkResult createGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, VkRenderPass renderPass,
                                    VulkanPipeline** ppPipeline);
    VkResult createComputePipeline(const ComputePipelineCreateInfo& createInfo, VulkanPipeline** ppPipeline);
    VkResult createDescriptorSetLayout(const std::vector<ResourcesBinding>& bindings,
                                       VulkanDescriptorSetLayout**          ppDescriptorSetLayout, bool enablePushDescriptor = false);

public:
    void destroyBuffer(VulkanBuffer* pBuffer);
    void destroyImage(VulkanImage* pImage);
    void destroyImageView(VulkanImageView* pImageView);
    void destoryRenderPass(VulkanRenderPass* pRenderpass);
    void destroySwapchain(VulkanSwapChain* pSwapchain);
    void destroyCommandPool(VulkanCommandPool* pPool);
    void destroyPipeline(VulkanPipeline* pipeline);
    void destroyDescriptorSetLayout(VulkanDescriptorSetLayout* pLayout);
    void destroySampler(VkSampler sampler);

public:
    VkResult flushMemory(VkDeviceMemory memory, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    VkResult invalidateMemory(VkDeviceMemory memory, VkDeviceSize size = 0, VkDeviceSize offset = VK_WHOLE_SIZE);
    VkResult bindMemory(VulkanBuffer* pBuffer, VkDeviceSize offset = 0);
    VkResult bindMemory(VulkanImage* pImage, VkDeviceSize offset = 0);
    VkResult mapMemory(VulkanBuffer* pBuffer, void* mapped = nullptr, VkDeviceSize offset = 0,
                       VkDeviceSize size = VK_WHOLE_SIZE);
    void     unMapMemory(VulkanBuffer* pBuffer);

public:
    VulkanCommandPool* getCommandPoolWithQueue(VulkanQueue* queue);
    VulkanQueue*       getQueueByFlags(QueueTypeFlags flags, uint32_t queueIndex = 0);
    VkResult           allocateCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer** ppCommandBuffers,
                                              VulkanQueue* pQueue);
    void               freeCommandBuffers(uint32_t commandBufferCount, VulkanCommandBuffer** ppCommandBuffers);

    VkResult waitIdle();
    VkResult waitForFence(const std::vector<VkFence>& fences, bool waitAll = true, uint32_t timeout = UINT32_MAX);
    VulkanPhysicalDevice*    getPhysicalDevice() const;
    VkFormat                 getDepthFormat() const;
    VkPhysicalDeviceFeatures getFeatures() const { return m_supportedFeatures; }

private:
    VkPhysicalDeviceFeatures m_supportedFeatures{};
    VulkanPhysicalDevice*    m_physicalDevice{};
    std::vector<QueueFamily> m_queues;
    QueueFamilyCommandPools  m_commandPools;
};

}  // namespace aph

#endif  // VKLDEVICE_H_
