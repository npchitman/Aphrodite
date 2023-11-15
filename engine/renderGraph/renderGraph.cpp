#include "renderGraph.h"
#include "threads/taskManager.h"

namespace aph
{
RenderPass::RenderPass(RenderGraph* pRDG, uint32_t index, QueueType queueType, std::string_view name) :
    m_pRenderGraph(pRDG),
    m_index(index),
    m_queueType(queueType),
    m_name(name)
{
    APH_ASSERT(pRDG);
}

PassBufferResource* RenderPass::addStorageBufferInput(const std::string& name, vk::Buffer* pBuffer)
{
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    res->addUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_STORAGE_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.storageBufferIn.push_back(res);

    if(pBuffer)
    {
        m_pRenderGraph->importResource(name, pBuffer);
    }

    return res;
}

PassBufferResource* RenderPass::addUniformBufferInput(const std::string& name, vk::Buffer* pBuffer)
{
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addReadPass(this);
    res->addUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::UniformBuffer;
    m_res.uniformBufferIn.push_back(res);

    if(pBuffer)
    {
        m_pRenderGraph->importResource(name, pBuffer);
    }

    return res;
}

PassBufferResource* RenderPass::addBufferOutput(const std::string& name)
{
    auto* res = static_cast<PassBufferResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Buffer));
    res->addWritePass(this);
    res->addUsage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_WRITE_BIT);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.storageBufferOut.push_back(res);

    return res;
}
PassImageResource* RenderPass::addTextureOutput(const std::string& name)
{
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_STORAGE_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

    m_res.resourceStateMap[res] = ResourceState::UnorderedAccess;
    m_res.textureOut.push_back(res);

    return res;
}

PassImageResource* RenderPass::addTextureInput(const std::string& name, vk::Image* pImage)
{
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->addReadPass(this);
    res->addUsage(VK_IMAGE_USAGE_SAMPLED_BIT);
    res->addAccessFlags(VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

    m_res.resourceStateMap[res] = ResourceState::ShaderResource;
    m_res.textureIn.push_back(res);

    if(pImage)
    {
        m_pRenderGraph->importResource(name, pImage);
    }

    return res;
}

PassImageResource* RenderPass::setColorOutput(const std::string& name, const PassImageInfo& info)
{
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    m_res.resourceStateMap[res] = ResourceState::RenderTarget;
    m_res.colorOut.push_back(res);
    return res;
}

PassImageResource* RenderPass::setDepthStencilOutput(const std::string& name, const PassImageInfo& info)
{
    auto* res = static_cast<PassImageResource*>(m_pRenderGraph->getResource(name, PassResource::Type::Image));
    res->setInfo(info);
    res->addWritePass(this);
    res->addUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_res.resourceStateMap[res] = ResourceState::DepthStencil;
    m_res.depthOut              = res;
    return res;
}

RenderPass* RenderGraph::getPass(const std::string& name)
{
    if(m_declareData.passMap.contains(name))
    {
        return m_declareData.passes[m_declareData.passMap[name]];
    }
    return nullptr;
}
RenderPass* RenderGraph::createPass(const std::string& name, QueueType queueType)
{
    if(m_declareData.passMap.contains(name))
    {
        return m_declareData.passes[m_declareData.passMap[name]];
    }

    auto  index = m_declareData.passes.size();
    auto* pass  = m_resourcePool.renderPass.allocate(this, index, queueType, name);
    m_declareData.passes.emplace_back(pass);
    m_declareData.passMap[name.data()] = index;
    return pass;
}

RenderGraph::RenderGraph(vk::Device* pDevice) : m_pDevice(pDevice)
{
}

void RenderGraph::build(const std::string& output)
{
    for(auto* pass : m_declareData.passes)
    {
        for(auto colorAttachment : pass->m_res.colorOut)
        {
            if(!m_buildData.image.contains(colorAttachment))
            {
                vk::Image*          pImage = {};
                vk::ImageCreateInfo createInfo{
                    .extent    = colorAttachment->getInfo().extent,
                    .usage     = colorAttachment->getUsage(),
                    .domain    = ImageDomain::Device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = colorAttachment->getInfo().format,
                };
                if(!output.empty() && m_declareData.resourceMap.contains(output))
                {
                    createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                }
                APH_CHECK_RESULT(m_pDevice->create(createInfo, &pImage));
                m_buildData.image[colorAttachment] = pImage;
            }
        }

        {
            auto depthAttachment = pass->m_res.depthOut;
            if(depthAttachment && !m_buildData.image.contains(depthAttachment))
            {
                vk::Image*          pImage = {};
                vk::ImageCreateInfo createInfo{
                    .extent    = depthAttachment->getInfo().extent,
                    .usage     = depthAttachment->getUsage(),
                    .domain    = ImageDomain::Device,
                    .imageType = VK_IMAGE_TYPE_2D,
                    .format    = depthAttachment->getInfo().format,
                };
                APH_CHECK_RESULT(m_pDevice->create(createInfo, &pImage));
                m_buildData.image[depthAttachment] = pImage;
            }
        }
    }
}

RenderGraph::~RenderGraph()
{
    for(auto* res : m_declareData.resources)
    {
        if(!(res->getFlags() & PASS_RESOURCE_EXTERNAL))
        {
            auto pImage = m_buildData.image[res];
            m_pDevice->destroy(pImage);
        }
    }
}

PassResource* RenderGraph::importResource(const std::string& name, vk::Buffer* pBuffer)
{
    auto res = getResource(name, PassResource::Type::Buffer);
    APH_ASSERT(!m_buildData.buffer.contains(res));
    res->addFlags(PASS_RESOURCE_EXTERNAL);
    m_buildData.buffer[res] = pBuffer;
    return res;
}

PassResource* RenderGraph::importResource(const std::string& name, vk::Image* pImage)
{
    auto res = getResource(name, PassResource::Type::Image);
    APH_ASSERT(!m_buildData.image.contains(res));
    res->addFlags(PASS_RESOURCE_EXTERNAL);
    m_buildData.image[res] = pImage;
    return res;
}

PassResource* RenderGraph::getResource(const std::string& name, PassResource::Type type)
{
    if(m_declareData.resourceMap.contains(name))
    {
        auto res = m_declareData.resources.at(m_declareData.resourceMap[name]);
        APH_ASSERT(res->getType() == type);
        return res;
    }

    std::size_t   idx = m_declareData.resources.size();
    PassResource* res = {};
    switch(type)
    {
    case PassResource::Type::Image:
        res = m_resourcePool.passImageResource.allocate(type);
        break;
    case PassResource::Type::Buffer:
        res = m_resourcePool.passBufferResource.allocate(type);
        break;
    }

    APH_ASSERT(res);

    m_declareData.resources.emplace_back(res);
    m_declareData.resourceMap[name] = idx;
    return res;
}

void RenderGraph::execute(const std::string& output, vk::Fence* pFence, vk::SwapChain* pSwapChain)
{
    auto* queue = m_pDevice->getQueue(aph::QueueType::Graphics);

    std::vector<vk::QueueSubmitInfo> frameSubmitInfos{};

    auto&      taskMgr = m_taskManager;
    auto       taskgrp = taskMgr.createTaskGroup();
    std::mutex submitLock;

    build(output);

    auto pOutImage = m_buildData.image[m_declareData.resources[m_declareData.resourceMap[output]]];

    for(auto* pass : m_declareData.passes)
    {
        bool                           finalOut = false;
        std::vector<vk::Image*>        colorImages;
        std::vector<vk::ImageBarrier>  imageBarriers{};
        std::vector<vk::BufferBarrier> bufferBarriers{};
        vk::Image*                     pDepthImage = {};

        colorImages.reserve(pass->m_res.colorOut.size());
        for(PassImageResource* colorAttachment : pass->m_res.colorOut)
        {
            colorImages.push_back(m_buildData.image[colorAttachment]);
            auto& image = colorImages.back();
            if(pOutImage == image)
            {
                finalOut = true;
            }
        }
        if(pass->m_res.depthOut)
        {
            pDepthImage = m_buildData.image[pass->m_res.depthOut];
        }

        for(PassImageResource* textureIn : pass->m_res.textureIn)
        {
            auto& image = m_buildData.image[textureIn];
            if(image->getResourceState() != pass->m_res.resourceStateMap[textureIn])
            {
                imageBarriers.push_back({
                    .pImage       = image,
                    .currentState = image->getResourceState(),
                    .newState     = pass->m_res.resourceStateMap[textureIn],
                });
            }
        }

        for(PassBufferResource* bufferIn : pass->m_res.storageBufferIn)
        {
            auto& buffer = m_buildData.buffer[bufferIn];
            if(buffer->getResourceState() != pass->m_res.resourceStateMap[bufferIn])
            {
                bufferBarriers.push_back(vk::BufferBarrier{
                    .pBuffer      = buffer,
                    .currentState = buffer->getResourceState(),
                    .newState     = pass->m_res.resourceStateMap[bufferIn],
                });
            }
        }

        for(PassBufferResource* bufferIn : pass->m_res.uniformBufferIn)
        {
            auto& buffer = m_buildData.buffer[bufferIn];
            if(buffer->getResourceState() != pass->m_res.resourceStateMap[bufferIn])
            {
                bufferBarriers.push_back(vk::BufferBarrier{
                    .pBuffer      = buffer,
                    .currentState = buffer->getResourceState(),
                    .newState     = pass->m_res.resourceStateMap[bufferIn],
                });
            }
        }

        APH_ASSERT(!colorImages.empty());

        taskgrp->addTask(
            [this, pass, queue, &frameSubmitInfos, &submitLock, colorImages, pDepthImage, pSwapChain, finalOut,
             &bufferBarriers, &imageBarriers]() {
                auto& cmdPool = pass->m_res.pCmdPools;
                if(cmdPool == nullptr)
                {
                    cmdPool = m_pDevice->acquireCommandPool({queue, false});
                }
                cmdPool->reset();
                vk::CommandBuffer* pCmd = cmdPool->allocate();
                pCmd->begin();
                pCmd->setDebugName(pass->m_name);
                pCmd->insertDebugLabel({.name = pass->m_name, .color = {0.6f, 0.6f, 0.6f, 0.6f}});
                pCmd->insertBarrier(bufferBarriers, imageBarriers);
                pCmd->beginRendering(colorImages, pDepthImage);
                APH_ASSERT(pass->m_executeCB);
                pass->m_executeCB(pCmd);
                pCmd->endRendering();
                pCmd->end();

                // lock
                vk::QueueSubmitInfo submitInfo{
                    .commandBuffers   = {pCmd},
                    .waitSemaphores   = {},
                    .signalSemaphores = {},
                };

                if(pSwapChain && finalOut)
                {
                    vk::Semaphore* renderSem = m_pDevice->acquireSemaphore();
                    APH_CHECK_RESULT(pSwapChain->acquireNextImage(renderSem));
                    submitInfo.waitSemaphores.push_back(renderSem);
                }

                std::lock_guard<std::mutex> holder{submitLock};
                frameSubmitInfos.push_back(std::move(submitInfo));
            },
            pass->m_name);
    }
    taskMgr.submit(taskgrp);

    // submit && present
    {
        vk::Fence* frameFence = {};

        if(!pFence)
        {
            frameFence = m_pDevice->acquireFence(false);
        }
        else
        {
            frameFence = pFence;
        }
        frameFence->reset();

        vk::Semaphore* presentSem = m_pDevice->acquireSemaphore();

        taskMgr.wait();
        APH_CHECK_RESULT(queue->submit(frameSubmitInfos, frameFence));

        if(pSwapChain)
        {
            auto pSwapchainImage = pSwapChain->getImage();

            // transisiton && copy
            m_pDevice->executeSingleCommands(
                queue,
                [pOutImage, pSwapchainImage](auto* pCopyCmd) {
                    pCopyCmd->transitionImageLayout(pOutImage, ResourceState::CopySource);
                    pCopyCmd->transitionImageLayout(pSwapchainImage, ResourceState::CopyDest);

                    if(pOutImage->getWidth() == pSwapchainImage->getWidth() &&
                       pOutImage->getHeight() == pSwapchainImage->getHeight() &&
                       pOutImage->getDepth() == pSwapchainImage->getDepth())
                    {
                        VK_LOG_DEBUG("copy image to swapchain.");
                        pCopyCmd->copyImage(pOutImage, pSwapchainImage);
                    }
                    else
                    {
                        VK_LOG_DEBUG("blit image to swapchain.");
                        pCopyCmd->blitImage(pOutImage, pSwapchainImage);
                    }

                    pCopyCmd->transitionImageLayout(pSwapchainImage, ResourceState::Present);
                },
                {}, {presentSem});
            APH_CHECK_RESULT(pSwapChain->presentImage(queue, {presentSem}));
        }

        if(!pFence)
        {
            frameFence->wait();
            APH_CHECK_RESULT(m_pDevice->releaseFence(frameFence));
        }
    }
}

vk::Image* RenderGraph::getBuildResource(PassImageResource* pResource) const
{
    APH_ASSERT(m_buildData.image.contains(pResource));
    return m_buildData.image.at(pResource);
}
vk::Buffer* RenderGraph::getBuildResource(PassBufferResource* pResource) const
{
    APH_ASSERT(m_buildData.buffer.contains(pResource));
    return m_buildData.buffer.at(pResource);
}
}  // namespace aph
