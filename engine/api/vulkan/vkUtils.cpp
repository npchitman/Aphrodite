#include "vkUtils.h"
#include "volk.h"

#include "allocator/allocator.h"

namespace aph::vk::utils
{

std::string errorString(VkResult errorCode)
{
    switch(errorCode)
    {
#define STR(r) \
    case VK_##r: \
        return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}

VkShaderStageFlags VkCast(const std::vector<ShaderStage>& stages)
{
    VkShaderStageFlags flags{};
    for(const auto& stage : stages)
    {
        flags |= VkCast(stage);
    }
    return flags;
}

VkShaderStageFlagBits VkCast(ShaderStage stage)
{
    switch(stage)
    {
    case ShaderStage::VS:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::TCS:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::TES:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case ShaderStage::GS:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::FS:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::CS:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::TS:
        return VK_SHADER_STAGE_TASK_BIT_EXT;
    case ShaderStage::MS:
        return VK_SHADER_STAGE_MESH_BIT_EXT;
    default:
        return VK_SHADER_STAGE_ALL;
    }
}

VkImageAspectFlags getImageAspect(VkFormat format)
{
    switch(format)
    {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_D32_SFLOAT:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

VkSampleCountFlagBits getSampleCountFlags(uint32_t numSamples)
{
    if(numSamples <= 1)
    {
        return VK_SAMPLE_COUNT_1_BIT;
    }
    if(numSamples <= 2)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    if(numSamples <= 4)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if(numSamples <= 8)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if(numSamples <= 16)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if(numSamples <= 32)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    return VK_SAMPLE_COUNT_64_BIT;
}

VkDebugUtilsLabelEXT VkCast(const DebugLabel& label)
{
    VkDebugUtilsLabelEXT vkLabel{
        .sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext      = nullptr,
        .pLabelName = label.name.c_str(),
        .color      = {label.color[0], label.color[1], label.color[2], label.color[3]},
    };

    return vkLabel;
}

VkAccessFlags getAccessFlags(ResourceState state)
{
    VkAccessFlags ret = 0;
    if((state & ResourceState::CopySource) != 0)
    {
        ret |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if((state & ResourceState::CopyDest) != 0)
    {
        ret |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if((state & ResourceState::VertexBuffer) != 0)
    {
        ret |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if((state & ResourceState::UniformBuffer) != 0)
    {
        ret |= VK_ACCESS_UNIFORM_READ_BIT;
    }
    if((state & ResourceState::IndexBuffer) != 0)
    {
        ret |= VK_ACCESS_INDEX_READ_BIT;
    }
    if((state & ResourceState::UnorderedAccess) != 0)
    {
        ret |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    }
    if((state & ResourceState::IndirectArgument) != 0)
    {
        ret |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if((state & ResourceState::RenderTarget) != 0)
    {
        ret |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if((state & ResourceState::DepthStencil) != 0)
    {
        ret |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if((state & ResourceState::ShaderResource) != 0)
    {
        ret |= VK_ACCESS_SHADER_READ_BIT;
    }
    if((state & ResourceState::Present) != 0)
    {
        ret |= VK_ACCESS_MEMORY_READ_BIT;
    }
    if((state & ResourceState::AccelStructRead) != 0)
    {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    }
    if((state & ResourceState::AccelStructWrite) != 0)
    {
        ret |= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    }
    return ret;
}

VkImageLayout getImageLayout(ResourceState state)
{
    if((state & ResourceState::CopySource) != 0)
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if((state & ResourceState::CopyDest) != 0)
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    if((state & ResourceState::RenderTarget) != 0)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if((state & ResourceState::DepthStencil) != 0)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if((state & ResourceState::UnorderedAccess) != 0)
        return VK_IMAGE_LAYOUT_GENERAL;

    if((state & ResourceState::ShaderResource) != 0)
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if((state & ResourceState::Present) != 0)
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if((state & ResourceState::General) != 0)
        return VK_IMAGE_LAYOUT_GENERAL;

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkFormat VkCast(Format format)
{
    switch(format)
    {
    case Format::R_UN8:
        return VK_FORMAT_R8_UNORM;
    case Format::R_UI16:
        return VK_FORMAT_R16_UINT;
    case Format::R_UN16:
        return VK_FORMAT_R16_UNORM;
    case Format::R_F16:
        return VK_FORMAT_R16_SFLOAT;
    case Format::R_F32:
        return VK_FORMAT_R32_SFLOAT;

    case Format::RG_UN8:
        return VK_FORMAT_R8G8_UNORM;
    case Format::RG_UI16:
        return VK_FORMAT_R16G16_UINT;
    case Format::RG_UN16:
        return VK_FORMAT_R16G16_UNORM;
    case Format::RG_F16:
        return VK_FORMAT_R16G16_SFLOAT;
    case Format::RG_F32:
        return VK_FORMAT_R32G32_SFLOAT;

    case Format::RGB_UN8:
        return VK_FORMAT_R8G8B8_UNORM;
    case Format::RGB_UI32:
        return VK_FORMAT_R32G32B32_UINT;
    case Format::RGB_F16:
        return VK_FORMAT_R16G16B16_SFLOAT;
    case Format::RGB_F32:
        return VK_FORMAT_R32G32B32_SFLOAT;

    case Format::RGBA_UN8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::RGBA_UI32:
        return VK_FORMAT_R32G32B32A32_UINT;
    case Format::RGBA_F16:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format::RGBA_F32:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format::RGBA_SRGB8:
        return VK_FORMAT_R8G8B8A8_SRGB;

    case Format::BGRA_UN8:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::BGRA_SRGB8:
        return VK_FORMAT_B8G8R8A8_SRGB;

    case Format::ETC2_RGB8:
        return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case Format::ETC2_SRGB8:
        return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    case Format::BC7_RGBA:
        return VK_FORMAT_BC7_UNORM_BLOCK;

    case Format::Z_UN16:
        return VK_FORMAT_D16_UNORM;
    case Format::Z_UN24:
        return VK_FORMAT_X8_D24_UNORM_PACK32;
    case Format::Z_F32:
        return VK_FORMAT_D32_SFLOAT;
    case Format::Z_UN24_S_UI8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::Z_F32_S_UI8:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;

    default:
        return VK_FORMAT_UNDEFINED;
    }
}
Format getFormatFromVk(VkFormat format)
{
    switch(format)
    {
    case VK_FORMAT_R8_UNORM:
        return Format::R_UN8;
    case VK_FORMAT_R16_UINT:
        return Format::R_UI16;
    case VK_FORMAT_R16_UNORM:
        return Format::R_UN16;
    case VK_FORMAT_R16_SFLOAT:
        return Format::R_F16;
    case VK_FORMAT_R32_SFLOAT:
        return Format::R_F32;

    case VK_FORMAT_R8G8_UNORM:
        return Format::RG_UN8;
    case VK_FORMAT_R16G16_UINT:
        return Format::RG_UI16;
    case VK_FORMAT_R16G16_UNORM:
        return Format::RG_UN16;
    case VK_FORMAT_R16G16_SFLOAT:
        return Format::RG_F16;
    case VK_FORMAT_R32G32_SFLOAT:
        return Format::RG_F32;

    case VK_FORMAT_R8G8B8_UNORM:
        return Format::RGB_UN8;
    case VK_FORMAT_R32G32B32_UINT:
        return Format::RGB_UI32;
    case VK_FORMAT_R16G16B16_SFLOAT:
        return Format::RGB_F16;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return Format::RGB_F32;

    case VK_FORMAT_R8G8B8A8_UNORM:
        return Format::RGBA_UN8;
    case VK_FORMAT_R32G32B32A32_UINT:
        return Format::RGBA_UI32;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return Format::RGBA_F16;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return Format::RGBA_F32;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Format::RGBA_SRGB8;

    case VK_FORMAT_B8G8R8A8_UNORM:
        return Format::BGRA_UN8;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return Format::BGRA_SRGB8;

    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return Format::ETC2_RGB8;
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        return Format::ETC2_SRGB8;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return Format::BC7_RGBA;

    case VK_FORMAT_D16_UNORM:
        return Format::Z_UN16;
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return Format::Z_UN24;
    case VK_FORMAT_D32_SFLOAT:
        return Format::Z_F32;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Format::Z_UN24_S_UI8;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return Format::Z_F32_S_UI8;

    default:
        return Format::Undefined;
    }
}

Result getResult(VkResult result)
{
    switch(result)
    {
    case VK_SUCCESS:
        return Result::Success;
    default:
        return Result::RuntimeError;
    }
}
VkIndexType VkCast(IndexType indexType)
{
    switch(indexType)
    {
    case IndexType::UINT16:
        return VK_INDEX_TYPE_UINT16;
    case IndexType::UINT32:
        return VK_INDEX_TYPE_UINT32;
    default:
        APH_ASSERT(false);
        return VK_INDEX_TYPE_NONE_KHR;
    }
}

VkResult setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, std::string_view name)
{
#if APH_DEBUG
    if(name.empty())
    {
        return VK_SUCCESS;
    }
    const VkDebugUtilsObjectNameInfoEXT ni = {
        .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType   = type,
        .objectHandle = handle,
        .pObjectName  = name.data(),
    };
    return vkSetDebugUtilsObjectNameEXT(device, &ni);
#else
    return VK_SUCCESS;
#endif
}
VkImageAspectFlags getImageAspect(Format format)
{
    return getImageAspect(VkCast(format));
}
}  // namespace aph::vk::utils

namespace aph::vk
{

static VKAPI_ATTR void* VKAPI_CALL vkAphAlloc(void* pUserData, size_t size, size_t alignment,
                                              VkSystemAllocationScope allocationScope)
{
    return memory::aph_memalign(alignment, size);
}

static VKAPI_ATTR void* VKAPI_CALL vkAphRealloc(void* pUserData, void* pOriginal, size_t size, size_t alignment,
                                                VkSystemAllocationScope allocationScope)
{
    return memory::aph_realloc(pOriginal, size);
}

static VKAPI_ATTR void VKAPI_CALL vkAphFree(void* pUserData, void* pMemory)
{
    return memory::aph_free(pMemory);
}

const VkAllocationCallbacks* vkAllocator()
{
    static const VkAllocationCallbacks allocator = {
        .pfnAllocation   = vkAphAlloc,
        .pfnReallocation = vkAphRealloc,
        .pfnFree         = vkAphFree,
    };
    return &allocator;
}
}  // namespace aph::vk
