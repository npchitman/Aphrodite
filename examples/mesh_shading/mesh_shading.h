#pragma once

#include "aph_core.hpp"
#include "aph_renderer.hpp"

class mesh_shading : public aph::BaseApp
{
public:
    mesh_shading();

    void init() override;
    void load() override;
    void run() override;
    void unload() override;
    void finish() override;

    struct
    {
        uint32_t windowWidth  = {1440};
        uint32_t windowHeight = {900};
    } m_options;

    struct UniformData
    {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
    };

private:
    aph::vk::Buffer*        m_pMVPBuffer = {};
    aph::vk::ShaderProgram* m_pProgram   = {};

private:
    std::unique_ptr<aph::vk::Renderer> m_renderer        = {};
    aph::WSI*                          m_pWSI            = {};
    aph::vk::Device*                   m_pDevice         = {};
    aph::ResourceLoader*               m_pResourceLoader = {};
    aph::vk::SwapChain*                m_pSwapChain      = {};

private:
    aph::Timer  m_timer;
    UniformData m_mvpData;
};
