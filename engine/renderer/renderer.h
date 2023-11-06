#ifndef VULKAN_RENDERER_H_
#define VULKAN_RENDERER_H_

#include "api/vulkan/device.h"
#include "renderGraph/renderGraph.h"
#include "resource/resourceLoader.h"
#include "uiRenderer.h"

namespace aph
{

enum RenderConfigFlagBits
{
    RENDER_CFG_DEBUG       = (1 << 0),
    RENDER_CFG_UI          = (1 << 1),
    RENDER_CFG_DEFAULT_RES = (1 << 2),
    RENDER_CFG_WITHOUT_UI  = RENDER_CFG_DEBUG | RENDER_CFG_DEFAULT_RES,
    RENDER_CFG_ALL         = RENDER_CFG_DEFAULT_RES | RENDER_CFG_UI
#if defined(APH_DEBUG)
                     | RENDER_CFG_DEBUG
#endif
    ,
};
using RenderConfigFlags = uint32_t;

struct RenderConfig
{
    RenderConfigFlags flags     = RENDER_CFG_ALL;
    uint32_t          maxFrames = {2};
};
}  // namespace aph

namespace aph::vk
{
class Renderer
{
public:
    Renderer(WSI* wsi, const RenderConfig& config);
    ~Renderer();

    void nextFrame();

public:
    void load();
    void unload();
    void update(float deltaTime);

public:
    SwapChain*      getSwapchain() const { return m_pSwapChain; }
    ResourceLoader* getResourceLoader() const { return m_pResourceLoader.get(); }
    Instance*       getInstance() const { return m_pInstance; }
    Device*         getDevice() const { return m_pDevice.get(); }
    RenderGraph*    getGraph() { return m_frameGraph[m_frameIdx].get(); }
    UI*             getUI() const { return m_pUI; }

    WSI*     getWSI() const { return m_wsi; }
    uint32_t getWindowWidth() const { return m_wsi->getWidth(); };
    uint32_t getWindowHeight() const { return m_wsi->getHeight(); };

    const RenderConfig& getConfig() const { return m_config; }

protected:
    VkSampleCountFlagBits m_sampleCount = {VK_SAMPLE_COUNT_1_BIT};

    Instance*                       m_pInstance  = {};
    SwapChain*                      m_pSwapChain = {};
    std::unique_ptr<ResourceLoader> m_pResourceLoader;
    std::unique_ptr<Device>         m_pDevice = {};

    VkSurfaceKHR    m_surface       = {};
    VkPipelineCache m_pipelineCache = {};

protected:
    UI* m_pUI = {};

    WSI*         m_wsi    = {};
    RenderConfig m_config = {};

protected:
    std::vector<std::unique_ptr<RenderGraph>> m_frameGraph;
    uint32_t                                  m_frameIdx = {};
};
}  // namespace aph::vk

#endif  // VULKANRENDERER_H_
