#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "api/vulkan/shader.h"
#include "renderer/renderer.h"

namespace aph
{
class VulkanRenderer : public IRenderer
{
public:
    VulkanRenderer(std::shared_ptr<Window> window, const RenderConfig& config);
    ~VulkanRenderer();

    void beginFrame();
    void endFrame();

public:
    VulkanInstance*     getInstance() const { return m_pInstance; }
    VulkanDevice*       getDevice() const { return m_pDevice; }
    VkPipelineCache     getPipelineCache() { return m_pipelineCache; }
    VulkanSwapChain*    getSwapChain() { return m_pSwapChain; }
    VulkanShaderModule* getShaders(const std::filesystem::path& path);

    VulkanQueue* getGraphicsQueue() const { return m_queue.graphics; }
    VulkanQueue* getComputeQueue() const { return m_queue.compute; }
    VulkanQueue* getTransferQueue() const { return m_queue.transfer; }

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_8_BIT};

    VulkanInstance*  m_pInstance  = {};
    VulkanDevice*    m_pDevice    = {};
    VulkanSwapChain* m_pSwapChain = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    struct
    {
        VulkanQueue* graphics = {};
        VulkanQueue* compute  = {};
        VulkanQueue* transfer = {};
    } m_queue;

    std::unordered_map<std::string, VulkanShaderModule*> shaderModuleCaches = {};

protected:
    std::unique_ptr<VulkanSyncPrimitivesPool> m_pSyncPrimitivesPool = {};
    std::vector<VkSemaphore>                  m_renderSemaphore     = {};
    std::vector<VkSemaphore>                  m_presentSemaphore    = {};
    std::vector<VkFence>                      m_frameFences         = {};
    std::vector<VulkanCommandBuffer*>         m_commandBuffers      = {};

protected:
    uint32_t m_frameIdx     = {};
    float    m_frameTimer   = {};
    uint32_t m_lastFPS      = {};
    uint32_t m_frameCounter = {};

    std::chrono::time_point<std::chrono::high_resolution_clock> m_timer = {};
    std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTimestamp, m_tStart, m_tPrevEnd;

protected:
    bool updateUIDrawData(float deltaTime);
    void recordUIDraw(VulkanCommandBuffer* pCommandBuffer);
    struct UI
    {
        void resize(uint32_t width, uint32_t height);
        struct PushConstBlock
        {
            glm::vec2 scale;
            glm::vec2 translate;
        } pushConstBlock;

        bool visible = {true};
        bool updated = {false};

        VulkanImage*     pFontImage  = {};
        VkSampler        fontSampler = {};
        VkDescriptorPool pool        = {};
        VkRenderPass     renderPass  = {};
        VulkanPipeline*  pipeline    = {};

        VulkanBuffer* pVertexBuffer = {};
        VulkanBuffer* pIndexBuffer  = {};
        uint32_t      vertexCount   = {};
        uint32_t      indexCount    = {};

        VulkanDescriptorSetLayout* pSetLayout = {};
        VkDescriptorSet            set        = {};

        float scale = {1.1f};
    } m_ui;
};
}  // namespace aph

#endif  // VULKANRENDERER_H_
