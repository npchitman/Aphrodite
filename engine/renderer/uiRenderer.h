#ifndef UIRENDERER_H_
#define UIRENDERER_H_

#include "api/vulkan/device.h"
#include "resource/resourceLoader.h"

class ImGuiContext;

namespace aph::vk
{

class Renderer;

enum UIFlags
{
    UI_Docking,
    UI_Demo,
};
MAKE_ENUM_FLAG(uint32_t, UIFlags);

struct UICreateInfo
{
    Renderer*   pRenderer  = {};
    UIFlags     flags      = {};
    std::string configFile = {};
};

class UI
{
public:
    UI(const UICreateInfo& ci);
    ~UI();

    using UIUpdateCallback = std::function<void()>;
    void record(UIUpdateCallback && func)
    {
        m_upateCB = std::move(func);
    }
    void update();
    void load();
    void unload();

    void draw(CommandBuffer* pCmd);

private:
    WSI*          m_pWSI     = {};
    ImGuiContext* m_pContext = {};

    bool m_updated = {false};

    UIUpdateCallback m_upateCB = {};

private:
    Renderer* m_pRenderer = {};
    Device*   m_pDevice   = {};

    VkDescriptorPool m_pool = {};

    Queue* m_pDefaultQueue = {};

    uint32_t m_frameIdx       = 0;
    bool     m_showDemoWindow = false;
};
}  // namespace aph::vk

#endif  // UIRENDERER_H_
