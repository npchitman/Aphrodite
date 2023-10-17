#include "resourceLoader.h"
#include "api/vulkan/device.h"
#include "tinyimageformat.h"

#define TINYKTX_IMPLEMENTATION
#include "tinyktx.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <stb/stb_image.h>
#include <tinygltf/tiny_gltf.h>

namespace
{
inline bool loadKTX(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    if(!std::filesystem::exists(path))
    {
        CM_LOG_ERR("File does not exist: %s", path.c_str());
        return false;
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.is_open())
    {
        CM_LOG_ERR("Failed to open file: %s", path.c_str());
        return false;
    }

    const auto ktxDataSize = static_cast<std::uint64_t>(file.tellg());
    if(ktxDataSize > UINT32_MAX)
    {
        CM_LOG_ERR("File too large: %s", path.c_str());
        return false;
    }

    file.seekg(0);

    TinyKtx_Callbacks callbacks{[](void* user, char const* msg) { CM_LOG_ERR("%s", msg); },
                                [](void* user, size_t size) { return malloc(size); },
                                [](void* user, void* memory) { free(memory); },
                                [](void* user, void* buffer, size_t byteCount) {
                                    auto ifs = static_cast<std::ifstream*>(user);
                                    ifs->read((char*)buffer, byteCount);
                                    return (size_t)ifs->gcount();
                                },
                                [](void* user, int64_t offset) {
                                    auto ifs = static_cast<std::ifstream*>(user);
                                    ifs->seekg(offset);
                                    return !ifs->fail();
                                },
                                [](void* user) { return (int64_t)(static_cast<std::ifstream*>(user)->tellg()); }};

    TinyKtx_ContextHandle ctx        = TinyKtx_CreateContext(&callbacks, &file);
    bool                  headerOkay = TinyKtx_ReadHeader(ctx);
    if(!headerOkay)
    {
        TinyKtx_DestroyContext(ctx);
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = outCI;
    textureCI.extent                    = {
                           .width  = TinyKtx_Width(ctx),
                           .height = TinyKtx_Height(ctx),
                           .depth  = std::max(1U, TinyKtx_Depth(ctx)),
    };
    textureCI.arraySize = std::max(1U, TinyKtx_ArraySlices(ctx));
    textureCI.mipLevels = std::max(1U, TinyKtx_NumberOfMipmaps(ctx));
    textureCI.format    = aph::vk::utils::getFormatFromVk(
        (VkFormat)TinyImageFormat_ToVkFormat(TinyImageFormat_FromTinyKtxFormat(TinyKtx_GetFormat(ctx))));
    // textureCI.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureCI.sampleCount = 1;

    if(textureCI.format == aph::Format::Undefined)
    {
        TinyKtx_DestroyContext(ctx);
        return false;
    }

    if(TinyKtx_IsCubemap(ctx))
    {
        textureCI.arraySize *= 6;
        // textureCI.descriptorType |= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    TinyKtx_DestroyContext(ctx);

    return true;
}

inline bool loadPNGJPG(const std::filesystem::path& path, aph::vk::ImageCreateInfo& outCI, std::vector<uint8_t>& data)
{
    auto img = aph::utils::loadImageFromFile(path.c_str());

    if(img == nullptr)
    {
        return false;
    }

    aph::vk::ImageCreateInfo& textureCI = outCI;

    textureCI.extent = {
        .width  = img->width,
        .height = img->height,
        .depth  = 1,
    };

    textureCI.format = aph::Format::RGBA_UN8;

    data = img->data;

    return true;
}
}  // namespace

namespace
{
inline bool loadGLTF(aph::ResourceLoader* pLoader, const aph::GeometryLoadInfo& info, aph::Geometry** ppGeometry)
{
    auto path = std::filesystem::path{info.path};
    auto ext  = path.extension();

    bool               fileLoaded = false;
    tinygltf::Model    inputModel;
    tinygltf::TinyGLTF gltfContext;
    std::string        error, warning;

    if(ext == ".glb")
    {
        fileLoaded = gltfContext.LoadBinaryFromFile(&inputModel, &error, &warning, path);
    }
    else if(ext == ".gltf")
    {
        fileLoaded = gltfContext.LoadASCIIFromFile(&inputModel, &error, &warning, path);
    }

    if(fileLoaded)
    {
        // TODO gltf loading
        *ppGeometry = new aph::Geometry;

        // Iterate over each mesh
        for(const auto& mesh : inputModel.meshes)
        {
            for(const auto& primitive : mesh.primitives)
            {
                // Index buffer
                const tinygltf::Accessor&   indexAccessor   = inputModel.accessors[primitive.indices];
                const tinygltf::BufferView& indexBufferView = inputModel.bufferViews[indexAccessor.bufferView];
                const tinygltf::Buffer&     indexBuffer     = inputModel.buffers[indexBufferView.buffer];

                {
                    aph::vk::Buffer*    pIB;
                    aph::BufferLoadInfo loadInfo{
                        .data = (void*)(indexBuffer.data.data() + indexBufferView.byteOffset),
                        // TODO index type
                        .createInfo = {.size  = static_cast<uint32_t>(indexAccessor.count * sizeof(uint16_t)),
                                       .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT}};
                    pLoader->load(loadInfo, &pIB);
                    (*ppGeometry)->indexBuffer.push_back(pIB);
                }

                // Vertex buffers
                for(const auto& attrib : primitive.attributes)
                {
                    const tinygltf::Accessor&   accessor   = inputModel.accessors[attrib.second];
                    const tinygltf::BufferView& bufferView = inputModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer&     buffer     = inputModel.buffers[bufferView.buffer];

                    aph::vk::Buffer*    pVB;
                    aph::BufferLoadInfo loadInfo{
                        .data = (void*)(buffer.data.data() + bufferView.byteOffset),
                        .createInfo = {.size  = static_cast<uint32_t>(accessor.count * accessor.ByteStride(bufferView)),
                                       .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
                    };
                    pLoader->load(loadInfo, &pVB);
                    (*ppGeometry)->vertexBuffers.push_back(pVB);
                    (*ppGeometry)->mVertexStrides.push_back(accessor.ByteStride(bufferView));
                }

                // TODO: Load draw arguments, handle materials, optimize geometry etc.

            }  // End of iterating through primitives

        }  // End of iterating through meshes
    }
    else
    {
        CM_LOG_ERR("%s", error);
        return false;
    }
}

}  // namespace

namespace aph
{

ImageContainerType GetImageContainerType(const std::filesystem::path& path)
{
    if(path.extension() == ".ktx")
    {
        return ImageContainerType::Ktx;
    }

    if(path.extension() == ".png")
    {
        return ImageContainerType::Png;
    }

    if(path.extension() == ".jpg")
    {
        return ImageContainerType::Jpg;
    }

    CM_LOG_ERR("Unsupported image format.");

    return ImageContainerType::Default;
}

ResourceLoader::ResourceLoader(const ResourceLoaderCreateInfo& createInfo) :
    m_createInfo(createInfo),
    m_pDevice(createInfo.pDevice)
{
}

ResourceLoader::~ResourceLoader(){};

void ResourceLoader::load(const ImageLoadInfo& info, vk::Image** ppImage)
{
    std::filesystem::path path;
    std::vector<uint8_t>  data;
    vk::ImageCreateInfo   ci;
    if(info.pCreateInfo)
    {
        ci = *info.pCreateInfo;
    }

    if(std::holds_alternative<std::string>(info.data))
    {
        path = {std::get<std::string>(info.data)};

        auto containerType =
            info.containerType == ImageContainerType::Default ? GetImageContainerType(path) : info.containerType;

        switch(containerType)
        {
        case ImageContainerType::Ktx:
        {
            loadKTX(path, ci, data);
        }
        break;
        case ImageContainerType::Png:
        case ImageContainerType::Jpg:
        {
            loadPNGJPG(path, ci, data);
        }
        break;
        case ImageContainerType::Default:
        case ImageContainerType::Dds:
            APH_ASSERT(false);
        }
    }
    else if(std::holds_alternative<ImageInfo>(info.data))
    {
        auto img  = std::get<ImageInfo>(info.data);
        data      = img.data;
        ci.extent = {img.width, img.height, 1};
    }

    bool           genMipmap = ci.mipLevels > 1;
    const uint32_t width     = ci.extent.width;
    const uint32_t height    = ci.extent.height;

    // Load texture from image buffer
    vk::Buffer* stagingBuffer;
    {
        vk::BufferCreateInfo bufferCI{
            .size   = static_cast<uint32_t>(data.size()),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };
        m_pDevice->create(bufferCI, &stagingBuffer);

        m_pDevice->mapMemory(stagingBuffer);
        stagingBuffer->write(data.data());
        m_pDevice->unMapMemory(stagingBuffer);
    }

    vk::Image* image{};
    {
        auto imageCI = ci;
        imageCI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.domain = ImageDomain::Device;
        if(genMipmap)
        {
            imageCI.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        m_pDevice->create(imageCI, &image);

        m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](vk::CommandBuffer* cmd) {
            cmd->transitionImageLayout(image, aph::RESOURCE_STATE_COPY_DST);

            cmd->copyBufferToImage(stagingBuffer, image);
            if(genMipmap)
            {
                cmd->transitionImageLayout(image, aph::RESOURCE_STATE_COPY_SRC);
            }
        });

        m_pDevice->executeSingleCommands(QueueType::GRAPHICS, [&](vk::CommandBuffer* cmd) {
            if(genMipmap)
            {
                // generate mipmap chains
                for(int32_t i = 1; i < imageCI.mipLevels; i++)
                {
                    VkImageBlit imageBlit{};

                    // Source
                    imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.srcSubresource.layerCount = 1;
                    imageBlit.srcSubresource.mipLevel   = i - 1;
                    imageBlit.srcOffsets[1].x           = int32_t(width >> (i - 1));
                    imageBlit.srcOffsets[1].y           = int32_t(height >> (i - 1));
                    imageBlit.srcOffsets[1].z           = 1;

                    // Destination
                    imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    imageBlit.dstSubresource.layerCount = 1;
                    imageBlit.dstSubresource.mipLevel   = i;
                    imageBlit.dstOffsets[1].x           = int32_t(width >> i);
                    imageBlit.dstOffsets[1].y           = int32_t(height >> i);
                    imageBlit.dstOffsets[1].z           = 1;

                    VkImageSubresourceRange mipSubRange = {};
                    mipSubRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
                    mipSubRange.baseMipLevel            = i;
                    mipSubRange.levelCount              = 1;
                    mipSubRange.layerCount              = 1;

                    // Prepare current mip level as image blit destination
                    vk::ImageBarrier barrier{
                        .pImage             = image,
                        .currentState       = image->getResourceState(),
                        .newState           = RESOURCE_STATE_COPY_DST,
                        .subresourceBarrier = 1,
                        .mipLevel           = static_cast<uint8_t>(imageCI.mipLevels),
                    };

                    cmd->insertBarrier({barrier});
                    // Blit from previous level
                    cmd->blitImage(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);
                    barrier.currentState = image->getResourceState();
                    barrier.newState     = RESOURCE_STATE_COPY_SRC;
                    cmd->insertBarrier({barrier});
                }
            }

            cmd->transitionImageLayout(image, RESOURCE_STATE_SHADER_RESOURCE);
        });
    }

    m_pDevice->destroy(stagingBuffer);
    *ppImage = image;
}

void ResourceLoader::load(const BufferLoadInfo& info, vk::Buffer** ppBuffer)
{
    vk::BufferCreateInfo bufferCI = info.createInfo;

    // using staging buffer
    vk::Buffer* stagingBuffer{};
    {
        vk::BufferCreateInfo stagingCI{
            .size   = static_cast<uint32_t>(bufferCI.size),
            .usage  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .domain = BufferDomain::Host,
        };

        m_pDevice->create(stagingCI, &stagingBuffer);

        m_pDevice->mapMemory(stagingBuffer);
        stagingBuffer->write(info.data);
        m_pDevice->unMapMemory(stagingBuffer);
    }

    {
        bufferCI.domain = BufferDomain::Device;
        bufferCI.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        m_pDevice->create(bufferCI, ppBuffer);
    }

    m_pDevice->executeSingleCommands(
        QueueType::GRAPHICS, [&](vk::CommandBuffer* cmd) { cmd->copyBuffer(stagingBuffer, *ppBuffer, bufferCI.size); });

    m_pDevice->destroy(stagingBuffer);
}

void ResourceLoader::load(const ShaderLoadInfo& info, vk::Shader** ppShader)
{
    auto uuid = m_uuidGenerator.getUUID().str();

    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    };
    std::vector<uint32_t> spvCode;
    if(std::holds_alternative<std::string>(info.data))
    {
        std::filesystem::path path = std::get<std::string>(info.data);

        // TODO override with new load info
        if(m_shaderUUIDMap.count(path))
        {
            *ppShader = m_shaderModuleCaches.at(m_shaderUUIDMap.at(path)).get();
            return;
        }

        if(path.extension() == ".spv")
        {
            spvCode = vk::utils::loadSpvFromFile(path);
        }
        else if(vk::utils::getStageFromPath(path.c_str()) != ShaderStage::NA)
        {
            spvCode = vk::utils::loadGlslFromFile(path);
        }

        createInfo.codeSize = spvCode.size() * sizeof(spvCode[0]);
        createInfo.pCode    = spvCode.data();

        m_shaderUUIDMap[path] = uuid;
    }
    else if(std::holds_alternative<std::vector<uint32_t>>(info.data))
    {
        auto& code          = std::get<std::vector<uint32_t>>(info.data);
        createInfo.codeSize = code.size() * sizeof(code[0]);
        createInfo.pCode    = code.data();

        {
            spvCode = code;
        }
    }

    // TODO macro
    {
    }

    VkShaderModule handle;
    VK_CHECK_RESULT(vkCreateShaderModule(m_pDevice->getHandle(), &createInfo, vk::vkAllocator(), &handle));

    APH_ASSERT(!m_shaderModuleCaches.count(uuid));
    m_shaderModuleCaches[uuid] = std::make_unique<vk::Shader>(spvCode, handle, info.entryPoint);

    *ppShader = m_shaderModuleCaches[uuid].get();
}

void ResourceLoader::cleanup()
{
    for(const auto& [_, shaderModule] : m_shaderModuleCaches)
    {
        vkDestroyShaderModule(m_pDevice->getHandle(), shaderModule->getHandle(), vk::vkAllocator());
    }
}

void ResourceLoader::load(const GeometryLoadInfo& info, Geometry** ppGeometry)
{
    auto path = std::filesystem::path{info.path};
    auto ext  = path.extension();

    if(ext == ".glb" || ext == ".gltf")
    {
        loadGLTF(this, info, ppGeometry);
    }
    else
    {
        CM_LOG_ERR("Unsupported model file type: %s.", ext);
        APH_ASSERT(false);
    }
}
}  // namespace aph
