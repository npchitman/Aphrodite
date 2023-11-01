#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "renderer/renderer.h"
#include "resource/resourceLoader.h"
#include "uiRenderer.h"

namespace aph::vk
{
class Renderer : public IRenderer
{
public:
    Renderer(WSI* wsi, const RenderConfig& config);
    ~Renderer() override;

    void beginFrame() override;
    void endFrame() override;

public:
    void load() override;
    void unload() override;
    void update(float deltaTime) override;

public:
    std::unique_ptr<ResourceLoader> m_pResourceLoader;
    std::unique_ptr<Device>         m_pDevice    = {};
    SwapChain*                      m_pSwapChain = {};

public:
    void    submit(Queue* pQueue, QueueSubmitInfo submitInfos, Image* pPresentImage = nullptr);
    Shader* getShaders(const std::filesystem::path& path) const;
    Queue*  getDefaultQueue(QueueType type) const { return m_queue.at(type); }

    Semaphore* getRenderSemaphore() { return m_frameData[m_frameIdx].renderSemaphore; }
    Fence*     getFrameFence() { return m_frameData[m_frameIdx].fence; }

    CommandPool* acquireCommandPool(Queue* queue, bool transient = false);
    Semaphore*   acquireSemahpore();
    Fence*       acquireFence();
    Instance*    getInstance() const { return m_pInstance; }

    VkQueryPool getFrameQueryPool() const { return m_frameData[m_frameIdx].queryPool; }

    using CmdRecordCallBack = std::function<void(CommandBuffer* pCmdBuffer)>;
    void executeSingleCommands(Queue* queue, const CmdRecordCallBack&& func, Fence* pFence = nullptr)
    {
        auto           commandPool = acquireCommandPool(queue, true);
        CommandBuffer* cmd         = nullptr;
        APH_CHECK_RESULT(commandPool->allocate(1, &cmd));

        _VR(cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT));
        func(cmd);
        _VR(cmd->end());

        QueueSubmitInfo submitInfo{.commandBuffers = {cmd}};
        APH_CHECK_RESULT(queue->submit({submitInfo}, pFence));
        APH_CHECK_RESULT(queue->waitIdle());

        commandPool->free(1, &cmd);
    }

public:
    UI* pUI = {};

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance* m_pInstance = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

    std::unordered_map<QueueType, Queue*> m_queue;

protected:
    struct FrameData
    {
        Semaphore*  renderSemaphore = {};
        Fence*      fence           = {};
        VkQueryPool queryPool       = {};

        std::vector<CommandPool*> cmdPools;
        std::vector<Semaphore*>   semaphores;
        std::vector<Fence*>       fences;
    };
    std::vector<FrameData> m_frameData;

    void resetFrameData();

protected:
    uint32_t m_frameIdx       = {};
    double   m_frameTime      = {};
    double   m_framePerSecond = {};
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
