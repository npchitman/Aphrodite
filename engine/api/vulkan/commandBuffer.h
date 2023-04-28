#ifndef COMMANDBUFFER_H_
#define COMMANDBUFFER_H_

#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph::vk
{

class Device;
class CommandPool;
class Pipeline;
class Buffer;
class Image;

enum class CommandBufferState
{
    INITIAL,
    RECORDING,
    EXECUTABLE,
    PENDING,
    INVALID,
};

class CommandBuffer : public ResourceHandle<VkCommandBuffer>
{
public:
    CommandBuffer(Device* pDevice, CommandPool* pool, VkCommandBuffer handle, uint32_t queueFamilyIndices);

    ~CommandBuffer();

    VkResult begin(VkCommandBufferUsageFlags flags = 0);
    VkResult end();
    VkResult reset();

    void beginRendering(const VkRenderingInfo& renderingInfo);
    void endRendering();
    void setViewport(const VkViewport& viewport);
    void setSissor(const VkRect2D& scissor);
    void bindDescriptorSet(Pipeline* pPipeline, uint32_t firstSet, uint32_t descriptorSetCount,
                           const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount = 0,
                           const uint32_t* pDynamicOffset = nullptr);
    void bindPipeline(Pipeline* pPipeline);
    void bindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const Buffer* pBuffer,
                           const std::vector<VkDeviceSize>& offsets);
    void bindIndexBuffers(const Buffer* pBuffer, VkDeviceSize offset, VkIndexType indexType);
    void pushConstants(Pipeline* pPipeline, const std::vector<ShaderStage>& stages, uint32_t offset, uint32_t size,
                       const void* pValues);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                     uint32_t firstInstance);
    void pushDescriptorSet(Pipeline* pipeline, const std::vector<VkWriteDescriptorSet>& writes, uint32_t setIdx);
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize size);
    void transitionImageLayout(Image* image, VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkImageSubresourceRange* pSubResourceRange = nullptr,
                               VkPipelineStageFlags     srcStageMask      = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                               VkPipelineStageFlags     dstStageMask      = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    void copyBufferToImage(Buffer* buffer, Image* image, const std::vector<VkBufferImageCopy>& regions = {});
    void copyImage(Image* srcImage, Image* dstImage);
    void imageMemoryBarrier(Image* image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                            VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
                            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                            VkImageSubresourceRange subresourceRange);
    void blitImage(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter = VK_FILTER_LINEAR);

    uint32_t getQueueFamilyIndices() const;

private:
    Device*                m_pDevice          = {};
    const VolkDeviceTable* m_pDeviceTable     = {};
    CommandPool*           m_pool             = {};
    CommandBufferState     m_state            = {};
    bool                   m_submittedToQueue = {false};
    uint32_t               m_queueFamilyType  = {};
};
}  // namespace aph::vk

#endif  // COMMANDBUFFER_H_
