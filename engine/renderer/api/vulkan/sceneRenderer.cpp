#include "sceneRenderer.h"

#include "common/assetManager.h"

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/node.h"

#include "api/vulkan/device.h"

#include <glm/gtx/string_cast.hpp>

namespace aph
{
VulkanSceneRenderer::VulkanSceneRenderer(const std::shared_ptr<VulkanRenderer> &renderer) :
    m_pDevice(renderer->getDevice()),
    m_pRenderer(renderer)
{
}

void VulkanSceneRenderer::loadResources()
{
    _loadScene();

    _initSetLayout();
    _initSet();

    _initForward();
    _initPostFx();
}

void VulkanSceneRenderer::cleanupResources()
{
    for(auto *pipeline : m_pipelines)
    {
        m_pDevice->destroyPipeline(pipeline);
    }

    for(auto *setLayout : m_setLayouts)
    {
        m_pDevice->destroyDescriptorSetLayout(setLayout);
    }

    for(auto *texture : m_textures)
    {
        m_pDevice->destroyImage(texture);
    }

    for(const auto &images : m_images)
    {
        for(auto *image : images)
        {
            m_pDevice->destroyImage(image);
        }
    }

    for(auto *buffer : m_buffers)
    {
        m_pDevice->destroyBuffer(buffer);
    }

    for(const auto sampler : m_samplers)
    {
        vkDestroySampler(m_pDevice->getHandle(), sampler, nullptr);
    }
}

void VulkanSceneRenderer::recordDrawSceneCommands()
{
    VkExtent2D extent{
        .width = m_pRenderer->getWindowWidth(),
        .height = m_pRenderer->getWindowHeight(),
    };
    VkViewport viewport = aph::init::viewport(extent);
    VkRect2D scissor = aph::init::rect2D(extent);

    VkCommandBufferBeginInfo beginInfo = aph::init::commandBufferBeginInfo();

    uint32_t imageIdx = m_pRenderer->getCurrentImageIndex();
    uint32_t frameIdx = m_pRenderer->getCurrentFrameIndex();

    auto *commandBuffer = m_pRenderer->getDefaultCommandBuffer(frameIdx);

    commandBuffer->begin();

    // dynamic state
    commandBuffer->setViewport(viewport);
    commandBuffer->setSissor(scissor);

    // forward pass
    {
        VulkanImageView *pColorAttachment = m_images[IMAGE_FORWARD_COLOR][imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardColorAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pColorAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color{ { 0.1f, 0.1f, 0.1f, 1.0f } } },
        };

        VulkanImageView *pDepthAttachment = m_images[IMAGE_FORWARD_DEPTH][imageIdx]->getImageView();
        VkRenderingAttachmentInfo forwardDepthAttachmentInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = pDepthAttachment->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .clearValue = { .depthStencil{ 1.0f, 0 } },
        };

        VkRenderingInfo renderingInfo{
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea{
                .offset{ 0, 0 },
                .extent{ extent },
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &forwardColorAttachmentInfo,
            .pDepthAttachment = &forwardDepthAttachmentInfo,
        };

        commandBuffer->bindPipeline(m_pipelines[PIPELINE_GRAPHICS_FORWARD]);
        commandBuffer->bindDescriptorSet(m_pipelines[PIPELINE_GRAPHICS_FORWARD], 0, 1, &m_sceneSet);

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        commandBuffer->transitionImageLayout(pDepthAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        commandBuffer->beginRendering(renderingInfo);
        commandBuffer->bindVertexBuffers(0, 1, m_buffers[BUFFER_SCENE_VERTEX], { 0 });

        for(uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
        {
            const auto &node = m_meshNodeList[nodeId];
            auto mesh = node->getObject<Mesh>();
            commandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
                                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                         offsetof(ObjectInfo, nodeId), sizeof(ObjectInfo::nodeId), &nodeId);
            if(mesh->m_indexOffset > -1)
            {
                VkIndexType indexType = VK_INDEX_TYPE_UINT32;
                switch(mesh->m_indexType)
                {
                case IndexType::UINT16:
                    indexType = VK_INDEX_TYPE_UINT16;
                    break;
                case IndexType::UINT32:
                    indexType = VK_INDEX_TYPE_UINT32;
                    break;
                default:
                    assert("undefined behavior.");
                    break;
                }
                commandBuffer->bindIndexBuffers(m_buffers[BUFFER_SCENE_INDEX], 0, indexType);
            }
            for(const auto &subset : mesh->m_subsets)
            {
                if(subset.indexCount > 0)
                {
                    commandBuffer->pushConstants(m_pipelines[PIPELINE_GRAPHICS_FORWARD],
                                                 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                                 offsetof(ObjectInfo, materialId), sizeof(ObjectInfo::materialId),
                                                 &subset.materialIndex);
                    if(subset.hasIndices)
                    {
                        commandBuffer->drawIndexed(subset.indexCount, 1, mesh->m_indexOffset + subset.firstIndex,
                                                   mesh->m_vertexOffset, 0);
                    }
                    else
                    {
                        commandBuffer->draw(subset.vertexCount, 1, subset.firstVertex, 0);
                    }
                }
            }
        }
        commandBuffer->endRendering();

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                             VK_IMAGE_LAYOUT_GENERAL);
    }

    // post fx
    {
        VulkanImageView *pColorAttachment = m_pRenderer->getSwapChain()->getImage(imageIdx)->getImageView();

        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_GENERAL);
        commandBuffer->bindPipeline(m_pipelines[PIPELINE_COMPUTE_POSTFX]);

        {
            VkDescriptorImageInfo inputImageInfo{
                .imageView = m_images[IMAGE_FORWARD_COLOR][imageIdx]->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            VkDescriptorImageInfo outputImageInfo{
                .imageView = m_pRenderer->getSwapChain()->getImage(imageIdx)->getImageView()->getHandle(),
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            };
            std::vector<VkWriteDescriptorSet> writes{
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, &inputImageInfo),
                aph::init::writeDescriptorSet(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &outputImageInfo),
            };

            commandBuffer->pushDescriptorSet(m_pipelines[PIPELINE_COMPUTE_POSTFX], writes, 0);
        }

        commandBuffer->dispatch(pColorAttachment->getImage()->getWidth(), pColorAttachment->getImage()->getHeight(), 1);
        commandBuffer->transitionImageLayout(pColorAttachment->getImage(), VK_IMAGE_LAYOUT_GENERAL,
                                             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    commandBuffer->end();
}

void VulkanSceneRenderer::update(float deltaTime)
{
    for (uint32_t nodeId = 0; nodeId < m_meshNodeList.size(); nodeId++)
    {
        const auto& node = m_meshNodeList[nodeId];
        m_transformInfos[nodeId] = node->getTransform();
    }

    for (uint32_t idx = 0; idx < m_cameraList.size(); idx++)
    {
        const auto& camera = m_cameraList[idx];
        CameraInfo cameraData{
            .view = camera->getViewMatrix(),
            .proj = camera->getProjMatrix(),
            .viewPos = camera->getPosition(),
        };
        m_cameraInfos[idx] = cameraData;
    }

    // for (uint32_t idx = 0; idx < m_lightList.size(); idx++)
    // {
    //     const auto& light = m_lightList[idx];
    //     LightInfo lightData{
    //         .color = light->getColor(),
    //         .position = light->getPosition(),
    //         .direction = light->getDirection(),
    //     };
    //     m_lightInfos[idx] = lightData;
    // }

    {
        m_buffers[BUFFER_SCENE_TRANSFORM]->copyTo(m_transformInfos.data());
        m_buffers[BUFFER_SCENE_CAMERA]->copyTo(m_cameraInfos.data());
        // m_buffers[BUFFER_SCENE_LIGHT]->copyTo(m_lightInfos.data());
    }
}

void VulkanSceneRenderer::_initSet()
{
    m_sceneSet = m_setLayouts[SET_LAYOUT_SCENE]->allocateSet();

    SceneInfo info{
        .ambient = glm::vec4(m_scene->getAmbient(), 0.0f),
        .cameraCount = static_cast<uint32_t>(m_cameraInfos.size()),
        .lightCount = static_cast<uint32_t>(m_lightInfos.size()),
    };

    VkWriteDescriptorSetInlineUniformBlock writeDescriptorSetInlineUniformBlock{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_INLINE_UNIFORM_BLOCK_EXT,
        .dataSize = sizeof(SceneInfo),
        .pData = &info,
    };

    VkWriteDescriptorSet sceneInfoSetWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = &writeDescriptorSetInlineUniformBlock,
        .dstSet = m_sceneSet,
        .dstBinding = 0,
        .descriptorCount = sizeof(SceneInfo),
        .descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
    };

    std::vector<VkDescriptorImageInfo> m_textureInfos{};
    for(auto &texture : m_textures)
    {
        VkDescriptorImageInfo info{
            .imageView = texture->getImageView()->getHandle(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        m_textureInfos.push_back(info);
    }

    VkDescriptorBufferInfo materialBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_MATERIAL]->getHandle(),
                                               .offset = 0,
                                               .range = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo cameraBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_CAMERA]->getHandle(),
                                             .offset = 0,
                                             .range = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo lightBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_LIGHT]->getHandle(),
                                            .offset = 0,
                                            .range = VK_WHOLE_SIZE };

    VkDescriptorBufferInfo transformBufferInfo{ .buffer = m_buffers[BUFFER_SCENE_TRANSFORM]->getHandle(),
                                                .offset = 0,
                                                .range = VK_WHOLE_SIZE };

    std::vector<VkWriteDescriptorSet> writes{
        sceneInfoSetWrite,
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &transformBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &cameraBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &lightBufferInfo, 1),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4, m_textureInfos.data(),
                                      m_textureInfos.size()),
        aph::init::writeDescriptorSet(m_sceneSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, &materialBufferInfo, 1),
    };
    vkUpdateDescriptorSets(m_pDevice->getHandle(), writes.size(), writes.data(), 0, nullptr);
}

void VulkanSceneRenderer::_loadScene()
{
    std::queue<std::shared_ptr<SceneNode>> q;
    q.push(m_scene->getRootNode());

    while(!q.empty())
    {
        const auto node = q.front();
        q.pop();

        switch(node->m_attachType)
        {
        case ObjectType::MESH:
        {
            m_transformInfos.push_back(node->getTransform());
            m_meshNodeList.push_back(node);
        }
        break;
        case ObjectType::CAMERA:
        {
            auto object = node->getObject<Camera>();
            CameraInfo cameraData{
                .view = object->getViewMatrix(),
                .proj = object->getProjMatrix(),
                .viewPos = object->getPosition(),
            };
            m_cameraList.push_back(object);
            m_cameraInfos.push_back(cameraData);
        }
        break;
        case ObjectType::LIGHT:
        {
            auto object = node->getObject<Light>();
            LightInfo lightData{
                .color = object->getColor(),
                .position = object->getPosition(),
                .direction = object->getDirection(),
            };
            m_lightList.push_back(object);
            m_lightInfos.push_back(lightData);
        }
        break;
        default:
            assert("unattached scene node.");
            break;
        }

        for(const auto &subNode : node->children)
        {
            q.push(subNode);
        }
    }

    // create camera buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_cameraInfos.size() * sizeof(CameraInfo)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_CAMERA], m_cameraInfos.data(), true);
    }

    // create light buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_lightInfos.size() * sizeof(LightInfo)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_LIGHT], m_cameraInfos.data());
    }

    // create transform buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_transformInfos.size() * sizeof(glm::mat4)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .property = MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        };
        m_pDevice->createBuffer(createInfo, &m_buffers[BUFFER_SCENE_TRANSFORM], m_transformInfos.data(), true);
    }

    // create index buffer
    {
        auto &indicesList = m_scene->m_indices;
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(indicesList.size()),
            .alignment = 0,
            .usage = BUFFER_USAGE_INDEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_INDEX], indicesList.data());
    }

    // create vertex buffer
    {
        auto &verticesList = m_scene->m_vertices;
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(verticesList.size()),
            .alignment = 0,
            .usage = BUFFER_USAGE_VERTEX_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_VERTEX], verticesList.data());
    }

    // create material buffer
    {
        BufferCreateInfo createInfo{
            .size = static_cast<uint32_t>(m_scene->m_materials.size() * sizeof(Material)),
            .alignment = 0,
            .usage = BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };
        m_pDevice->createDeviceLocalBuffer(createInfo, &m_buffers[BUFFER_SCENE_MATERIAL], m_scene->m_materials.data());
    }

    // load scene image to gpu
    for(const auto &image : m_scene->m_images)
    {
        ImageCreateInfo createInfo{
            .extent = { image->width, image->height, 1 },
            .mipLevels = calculateFullMipLevels(image->width, image->height),
            .usage = IMAGE_USAGE_SAMPLED_BIT,
            .format = FORMAT_R8G8B8A8_UNORM,
            .tiling = IMAGE_TILING_OPTIMAL,
        };

        VulkanImage *texture{};
        m_pDevice->createDeviceLocalImage(createInfo, &texture, image->data);
        m_textures.push_back(texture);
    }
}

void VulkanSceneRenderer::_initPostFx()
{
    // build pipeline
    std::filesystem::path shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
    ComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_POSTFX] };
    pipelineCreateInfo.shaderMapList = {
        { VK_SHADER_STAGE_COMPUTE_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "postFX.comp.spv") },
    };
    VK_CHECK_RESULT(m_pDevice->createComputePipeline(pipelineCreateInfo, &m_pipelines[PIPELINE_COMPUTE_POSTFX]));
}

void VulkanSceneRenderer::_initForward()
{
    uint32_t imageCount = m_pRenderer->getSwapChain()->getImageCount();
    VkExtent2D imageExtent = m_pRenderer->getSwapChain()->getExtent();

    m_images[IMAGE_FORWARD_COLOR].resize(imageCount);
    m_images[IMAGE_FORWARD_DEPTH].resize(imageCount);

    // frame buffer
    for(auto idx = 0; idx < imageCount; idx++)
    {
        auto &colorImage = m_images[IMAGE_FORWARD_COLOR][idx];
        auto &depthImage = m_images[IMAGE_FORWARD_DEPTH][idx];

        {
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .imageType = IMAGE_TYPE_2D,
                .usage = IMAGE_USAGE_COLOR_ATTACHMENT_BIT | IMAGE_USAGE_STORAGE_BIT | IMAGE_USAGE_SAMPLED_BIT,
                .property = MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = FORMAT_B8G8R8A8_UNORM,
            };
            m_pDevice->createImage(createInfo, &colorImage);
        }

        {
            ImageCreateInfo createInfo{
                .extent = { imageExtent.width, imageExtent.height, 1 },
                .usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .property = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                .format = static_cast<Format>(m_pDevice->getDepthFormat()),
                .tiling = IMAGE_TILING_OPTIMAL,
            };
            VK_CHECK_RESULT(m_pDevice->createImage(createInfo, &depthImage));
        }

        m_pDevice->executeSingleCommands(QUEUE_GRAPHICS, [&](VulkanCommandBuffer *cmd) {
            cmd->transitionImageLayout(depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        });
    }

    // forward graphics pipeline
    {
        GraphicsPipelineCreateInfo pipelineCreateInfo{};
        auto shaderDir = AssetManager::GetShaderDir(ShaderAssetType::GLSL) / "default";
        std::vector<VkFormat> colorFormats = { m_pRenderer->getSwapChain()->getSurfaceFormat() };
        pipelineCreateInfo.renderingCreateInfo = VkPipelineRenderingCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = static_cast<uint32_t>(colorFormats.size()),
            .pColorAttachmentFormats = colorFormats.data(),
            .depthAttachmentFormat = m_pDevice->getDepthFormat(),
        };
        pipelineCreateInfo.setLayouts = { m_setLayouts[SET_LAYOUT_SCENE] };
        pipelineCreateInfo.constants.push_back(aph::init::pushConstantRange(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(ObjectInfo), 0));
        pipelineCreateInfo.shaderMapList = {
            { VK_SHADER_STAGE_VERTEX_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.vert.spv") },
            { VK_SHADER_STAGE_FRAGMENT_BIT, m_pRenderer->getShaderCache()->getShaders(shaderDir / "pbr.frag.spv") },
        };

        VK_CHECK_RESULT(
            m_pDevice->createGraphicsPipeline(pipelineCreateInfo, nullptr, &m_pipelines[PIPELINE_GRAPHICS_FORWARD]));
    }
}

void VulkanSceneRenderer::_initSetLayout()
{
    // scene
    {
        {
            VkSamplerCreateInfo samplerInfo = aph::init::samplerCreateInfo();
            samplerInfo.maxLod = calculateFullMipLevels(2048, 2048);
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VK_CHECK_RESULT(vkCreateSampler(m_pDevice->getHandle(), &samplerInfo, nullptr, &m_samplers[SAMP_TEXTURE]));
        }
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                                  sizeof(SceneInfo)),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                  VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, 4,
                                                  m_textures.size()),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 5,
                                                  1),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6, 1,
                                                  m_samplers.data()),
        };
        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_SCENE]);
    }

    // off screen texture
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings{
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0),
            aph::init::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1)
        };

        VkDescriptorSetLayoutCreateInfo createInfo = aph::init::descriptorSetLayoutCreateInfo(bindings);
        createInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
        m_pDevice->createDescriptorSetLayout(createInfo, &m_setLayouts[SET_LAYOUT_POSTFX]);
    }
}
}  // namespace aph
