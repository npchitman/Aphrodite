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
class ImageView;
class Sampler;

enum class CommandBufferState
{
    INITIAL,
    RECORDING,
    EXECUTABLE,
    PENDING,
    INVALID,
};

struct AttachmentInfo
{
    Image*                             image{};
    std::optional<VkImageLayout>       layout;
    std::optional<VkAttachmentLoadOp>  loadOp;
    std::optional<VkAttachmentStoreOp> storeOp;
    std::optional<VkClearValue>        clear;
};

struct ResourceBinding
{
    union
    {
        VkDescriptorBufferInfo buffer;
        VkDescriptorImageInfo  image;
        VkBufferView           bufferView;
    };
    VkDeviceSize     dynamicOffset;
    VkDescriptorType resType;
};

struct ResourceBindings
{
    std::optional<ResourceBinding> bindings[VULKAN_NUM_DESCRIPTOR_SETS][VULKAN_NUM_BINDINGS];
    uint8_t                        push_constant_data[VULKAN_PUSH_CONSTANT_SIZE];
    uint32_t                       dirty = 0;
};

struct IndexState
{
    VkBuffer     buffer;
    VkDeviceSize offset;
    VkIndexType  indexType;
};

struct VertexBindingState
{
    VkBuffer     buffers[VULKAN_NUM_VERTEX_BUFFERS];
    VkDeviceSize offsets[VULKAN_NUM_VERTEX_BUFFERS];
    uint32_t     dirty = 0;
};

struct CommandState
{
    Pipeline*                     pPipeline{};
    VkViewport                    viewport{};
    VkRect2D                      scissor{};
    std::vector<AttachmentInfo>   colorAttachments;
    std::optional<AttachmentInfo> depthAttachment;
    ResourceBindings              resourceBindings{};
};

class CommandBuffer : public ResourceHandle<VkCommandBuffer>
{
public:
    CommandBuffer(Device* pDevice, CommandPool* pool, VkCommandBuffer handle, uint32_t queueFamilyIndices);
    ~CommandBuffer();

    VkResult begin(VkCommandBufferUsageFlags flags = 0);
    VkResult end();
    VkResult reset();

    void bindBuffer(uint32_t set, uint32_t binding, ResourceType type, Buffer* buffer, VkDeviceSize offset = 0,
                    VkDeviceSize size = VK_WHOLE_SIZE);
    void bindTexture(uint32_t set, uint32_t binding, ResourceType type, ImageView* imageview, VkImageLayout layout,
                     Sampler* sampler = nullptr);

    void beginRendering(VkRect2D renderArea, const std::vector<Image*>& colors, Image* depth = nullptr);
    void beginRendering(VkRect2D renderArea, const std::vector<AttachmentInfo>& colors, const AttachmentInfo& depth);
    void endRendering();

    void setViewport(const VkExtent2D& extent);
    void setScissor(const VkExtent2D& extent);
    void setViewport(const VkViewport& viewport);
    void setScissor(const VkRect2D& scissor);
    void bindDescriptorSet(const std::vector<VkDescriptorSet>& pDescriptorSets, uint32_t firstSet = 0);
    void bindDescriptorSet(uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets,
                           uint32_t dynamicOffsetCount = 0, const uint32_t* pDynamicOffset = nullptr);
    void bindPipeline(Pipeline* pPipeline);
    void bindVertexBuffers(Buffer* pBuffer, uint32_t binding = 0, uint32_t offset = 0);
    void bindIndexBuffers(Buffer* pBuffer, VkDeviceSize offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void pushConstants(uint32_t offset, uint32_t size, const void* pValues);

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset,
                     uint32_t firstInstance);
    void dispatch(Buffer* pBuffer, VkDeviceSize offset = 0);
    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void draw(Buffer* pBuffer, VkDeviceSize offset = 0, uint32_t drawCount = 1,
              uint32_t stride = sizeof(VkDrawIndirectCommand));

    void copyBuffer(Buffer* srcBuffer, Buffer* dstBuffer, VkDeviceSize size);
    void transitionImageLayout(Image* image, VkImageLayout newLayout,
                               VkImageSubresourceRange* pSubResourceRange = nullptr,
                               VkPipelineStageFlags2    srcStageMask      = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                               VkPipelineStageFlags2    dstStageMask      = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
    void copyBufferToImage(Buffer* buffer, Image* image, const std::vector<VkBufferImageCopy>& regions = {});
    void copyImage(Image* srcImage, Image* dstImage);
    void blitImage(Image* srcImage, VkImageLayout srcImageLayout, Image* dstImage, VkImageLayout dstImageLayout,
                   uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter = VK_FILTER_LINEAR);

    uint32_t getQueueFamilyIndices() const;

private:
    void                   flushComputeCommand();
    void                   flushGraphicsCommand();
    Device*                m_pDevice          = {};
    const VolkDeviceTable* m_pDeviceTable     = {};
    CommandPool*           m_pool             = {};
    CommandBufferState     m_state            = {};
    bool                   m_submittedToQueue = {false};
    uint32_t               m_queueFamilyType  = {};

private:
    IndexState         m_indexState         = {};
    VertexBindingState m_vertexBindingState = {};
    CommandState       m_commandState       = {};

private:
    uint32_t m_dirtySet = 0U;
};
}  // namespace aph::vk

#endif  // COMMANDBUFFER_H_
