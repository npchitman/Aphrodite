#ifndef VKLUTILS_H_
#define VKLUTILS_H_

#if defined(__linux__)
    #define VK_USE_PLATFORM_XCB_KHR
#endif
#define VK_NO_PROTOTYPES
#include <volk.h>
#include "vkInit.h"
#include "common/common.h"
#include "common/logger.h"

#include "allocator/objectPool.h"

#include "../gpuResource.h"

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

namespace aph::vk
{
#ifdef APH_DEBUG
    #define _VR(f) \
        { \
            VkResult res = (f); \
            if(res != VK_SUCCESS) \
            { \
                VK_LOG_ERR("Fatal : VkResult is \"%s\" in %s at line %s", aph::vk::utils::errorString(res), __FILE__, \
                           __LINE__); \
                std::abort(); \
            } \
        }
#else
    #define _VR(f) (f)
#endif
}  // namespace aph::vk

namespace aph::vk::utils
{
std::string           errorString(VkResult errorCode);
VkImageAspectFlags    getImageAspect(VkFormat format);
VkImageAspectFlags    getImageAspect(Format format);
VkSampleCountFlagBits getSampleCountFlags(uint32_t numSamples);
VkAccessFlags         getAccessFlags(ResourceState state);
VkImageLayout         getImageLayout(ResourceState state);
Format                getFormatFromVk(VkFormat format);
Result                getResult(VkResult result);
VkResult              setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, std::string_view name);
}  // namespace aph::vk::utils

// convert
namespace aph::vk::utils
{
VkShaderStageFlagBits VkCast(ShaderStage stage);
VkShaderStageFlags    VkCast(const std::vector<ShaderStage>& stages);
VkDebugUtilsLabelEXT  VkCast(const DebugLabel& label);
VkFormat              VkCast(Format format);
VkIndexType           VkCast(IndexType indexType);
VkCompareOp           VkCast(CompareOp compareOp);
}  // namespace aph::vk::utils

namespace aph
{
constexpr unsigned VULKAN_NUM_DESCRIPTOR_SETS           = 4;
constexpr unsigned VULKAN_NUM_BINDINGS                  = 32;
constexpr unsigned VULKAN_NUM_BINDINGS_BINDLESS_VARYING = 16 * 1024;
constexpr unsigned VULKAN_NUM_ATTACHMENTS               = 8;
constexpr unsigned VULKAN_NUM_VERTEX_ATTRIBS            = 16;
constexpr unsigned VULKAN_NUM_VERTEX_BUFFERS            = 4;
constexpr unsigned VULKAN_PUSH_CONSTANT_SIZE            = 128;
constexpr unsigned VULKAN_MAX_UBO_SIZE                  = 16 * 1024;
constexpr unsigned VULKAN_NUM_USER_SPEC_CONSTANTS       = 8;
constexpr unsigned VULKAN_NUM_INTERNAL_SPEC_CONSTANTS   = 4;
constexpr unsigned VULKAN_NUM_TOTAL_SPEC_CONSTANTS =
    VULKAN_NUM_USER_SPEC_CONSTANTS + VULKAN_NUM_INTERNAL_SPEC_CONSTANTS;
constexpr unsigned VULKAN_NUM_SETS_PER_POOL    = 16;
constexpr unsigned VULKAN_DESCRIPTOR_RING_SIZE = 8;
}  // namespace aph

namespace aph::vk
{
const VkAllocationCallbacks* vkAllocator();
}  // namespace aph::vk

#endif  // VKLUTILS_H_
