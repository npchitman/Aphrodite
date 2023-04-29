#include "shader.h"
#include "device.h"

namespace aph::vk
{
std::unique_ptr<ShaderModule> ShaderModule::Create(Device* pDevice, const std::vector<char>& code,
                                                   const std::string& entrypoint)
{
    VkShaderModuleCreateInfo createInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<const uint32_t*>(code.data()),
    };

    VkShaderModule handle;
    VK_CHECK_RESULT(vkCreateShaderModule(pDevice->getHandle(), &createInfo, nullptr, &handle));

    auto instance = std::unique_ptr<ShaderModule>(new ShaderModule(code, handle, entrypoint));
    return instance;
}

ShaderModule::ShaderModule(std::vector<char> code, VkShaderModule shaderModule, std::string entrypoint) :
    m_entrypoint(std::move(entrypoint)),
    m_code(std::move(code))
{
    getHandle() = shaderModule;
}
}  // namespace aph::vk
