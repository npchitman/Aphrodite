#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "common/threadPool.h"
#include "api/gpuResource.h"
#include "vkUtils.h"

namespace aph
{

using InstanceCreationFlags = uint32_t;

struct InstanceCreateInfo
{
    const char*              pApplicationName = "Aphrodite";
    std::vector<const char*> enabledLayers{};
    std::vector<const char*> enabledExtensions{};
};

class VulkanPhysicalDevice;

class VulkanInstance : public ResourceHandle<VkInstance>
{
private:
    VulkanInstance() = default;

public:
    static VkResult Create(const InstanceCreateInfo& createInfo, VulkanInstance** ppInstance);

    static void Destroy(VulkanInstance* pInstance);

    ThreadPool*           getThreadPool() { return m_threadPool; }
    VulkanPhysicalDevice* getPhysicalDevices(uint32_t idx) { return m_physicalDevices[idx].get(); }

private:
    VkDebugUtilsMessengerEXT                           m_debugMessenger{};
    std::vector<const char*>                           m_supportedInstanceExtensions{};
    std::vector<std::string>                           m_validationLayers{};
    std::vector<std::unique_ptr<VulkanPhysicalDevice>> m_physicalDevices{};
    ThreadPool*                                        m_threadPool{};
};
}  // namespace aph

#endif  // INSTANCE_H_
