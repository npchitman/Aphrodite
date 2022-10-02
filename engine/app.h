#ifndef VULKANBASE_H_
#define VULKANBASE_H_

#include "vkl.hpp"

namespace vkl {

struct MouseData {
    float lastX;
    float lastY;
    bool  firstMouse      = true;
    bool  isCursorDisable = false;
    MouseData(float lastXin, float lastYin)
        : lastX(lastXin), lastY(lastYin) {
    }
};

class vklApp {
public:
    vklApp(std::string sessionName = "", uint32_t winWidth = 800, uint32_t winHeight = 600);
    virtual ~vklApp() = default;

public:
    void init();
    void run();
    void finish();

protected:
    const std::filesystem::path assetDir      = "assets";
    const std::filesystem::path glslShaderDir = assetDir / "shaders/glsl";
    const std::filesystem::path textureDir    = assetDir / "textures";
    const std::filesystem::path modelDir      = assetDir / "models";

protected:
    struct {
        bool           enableValidationLayers = true;
        bool           enableUI               = false;
        const uint32_t max_frames             = 2;
    } m_settings;

protected:
    void initWindow();
    void initRenderer();
    void cleanup();

protected:
    virtual void initDerive() = 0;
    virtual void drawFrame()  = 0;

protected:
    virtual void keyboardHandleDerive(int key, int scancode, int action, int mods);
    virtual void mouseHandleDerive(double xposIn, double yposIn);

protected:
    const std::string m_sessionName;

    Window m_window;

    MouseData m_mouseData;
    float     m_deltaTime;

    std::shared_ptr<Camera>   m_defaultCamera = nullptr;
    std::unique_ptr<Renderer> m_renderer;

    bool               m_framebufferResized = false;
    vkl::DeletionQueue m_deletionQueue;

    CursorPosFunc func;
};
} // namespace vkl

#endif // VULKANBASE_H_
