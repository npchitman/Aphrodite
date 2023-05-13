#include "shader.h"
#include "device.h"
#include "spirv_cross.hpp"

namespace aph::vk
{

static void updateArrayInfo(ResourceLayout& layout, const spirv_cross::SPIRType& type, unsigned set, unsigned binding)
{
    auto& size = layout.setShaderLayouts[set].arraySize[binding];
    if(!type.array.empty())
    {
        if(type.array.size() != 1)
            VK_LOG_ERR("Array dimension must be 1.");
        else if(!type.array_size_literal.front())
            VK_LOG_ERR("Array dimension must be a literal.");
        else
        {
            if(type.array.front() == 0)
            {
                if(binding != 0)
                    VK_LOG_ERR("Bindless textures can only be used with binding = 0 in a set.");

                if(type.basetype != spirv_cross::SPIRType::Image || type.image.dim == spv::DimBuffer)
                    VK_LOG_ERR("Can only use bindless for sampled images.");
                else
                    layout.bindlessSetMask |= 1u << set;

                size = ShaderLayout::UNSIZED_ARRAY;
            }
            else if(size && size != type.array.front())
                VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
            else if(type.array.front() + binding > VULKAN_NUM_BINDINGS)
                VK_LOG_ERR("Binding array will go out of bounds.");
            else
                size = uint8_t(type.array.front());
        }
    }
    else
    {
        if(size && size != 1)
            VK_LOG_ERR("Array dimension for (%u, %u) is inconsistent.", set, binding);
        size = 1;
    }
};

// create descriptor set layout
static VkDescriptorSetLayout createDescriptorSetLayout(Device* m_pDevice, const ShaderLayout& layout,
                                                       const ImmutableSampler* const*     pImmutableSamplers,
                                                       const uint32_t*                    stageForBinds,
                                                       std::vector<VkDescriptorPoolSize>& poolSize)
{
    VkDescriptorSetLayout set_layout{};

    VkDescriptorSetLayoutCreateInfo           info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    VkSampler                                 vkImmutableSamplers[VULKAN_NUM_BINDINGS] = {};
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    // VkDescriptorBindingFlagsEXT               binding_flags = 0;

    // VkDescriptorSetLayoutBindingFlagsCreateInfoEXT flags = {
    //     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT};

    for(unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
    {
        uint32_t stages = stageForBinds[i];
        if(stages == 0)
            continue;

        unsigned array_size = layout.arraySize[i];
        unsigned pool_array_size;
        if(array_size == ShaderLayout::UNSIZED_ARRAY)
        {
            array_size      = VULKAN_NUM_BINDINGS_BINDLESS_VARYING;
            pool_array_size = array_size;
        }
        else
            pool_array_size = array_size * VULKAN_NUM_SETS_PER_POOL;

        unsigned types = 0;
        if(layout.sampledImageMask & (1u << i))
        {
            if((layout.immutableSamplerMask & (1u << i)) && pImmutableSamplers && pImmutableSamplers[i])
                vkImmutableSamplers[i] = pImmutableSamplers[i]->getSampler()->getHandle();

            bindings.push_back({i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_size, stages,
                                vkImmutableSamplers[i] != VK_NULL_HANDLE ? &vkImmutableSamplers[i] : nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pool_array_size});
            types++;
        }

        if(layout.sampledTexelBufferMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, pool_array_size});
            types++;
        }

        if(layout.storageTexelBufferMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, pool_array_size});
            types++;
        }

        if(layout.storageImageMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, pool_array_size});
            types++;
        }

        if(layout.uniformBufferMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pool_array_size});
            types++;
        }

        if(layout.storageBufferMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pool_array_size});
            types++;
        }

        if(layout.inputAttachmentMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, pool_array_size});
            types++;
        }

        if(layout.separateImageMask & (1u << i))
        {
            bindings.push_back({i, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, array_size, stages, nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, pool_array_size});
            types++;
        }

        if(layout.samplerMask & (1u << i))
        {
            if((layout.immutableSamplerMask & (1u << i)) && pImmutableSamplers && pImmutableSamplers[i])
                vkImmutableSamplers[i] = pImmutableSamplers[i]->getSampler()->getHandle();

            bindings.push_back({i, VK_DESCRIPTOR_TYPE_SAMPLER, array_size, stages,
                                vkImmutableSamplers[i] != VK_NULL_HANDLE ? &vkImmutableSamplers[i] : nullptr});
            poolSize.push_back({VK_DESCRIPTOR_TYPE_SAMPLER, pool_array_size});
            types++;
        }

        (void)types;
        APH_ASSERT(types <= 1 && "Descriptor set aliasing!");
    }

    if(!bindings.empty())
    {
        info.bindingCount = bindings.size();
        info.pBindings    = bindings.data();
    }

#ifdef APH_DEBUG
    VK_LOG_INFO("Creating descriptor set layout.");
#endif
    auto table = m_pDevice->getDeviceTable();
    if(table->vkCreateDescriptorSetLayout(m_pDevice->getHandle(), &info, nullptr, &set_layout) != VK_SUCCESS)
        VK_LOG_ERR("Failed to create descriptor set layout.");

    return set_layout;
};

std::unique_ptr<Shader> Shader::Create(Device* pDevice, const std::filesystem::path& path,
                                       const std::string& entrypoint, const ResourceLayout* pLayout)
{
    std::vector<uint32_t> spvCode;
    if(path.extension() == ".spv")
    {
        spvCode = utils::loadSpvFromFile(path);
    }
    else if(utils::getStageFromPath(path.c_str()) != ShaderStage::NA)
    {
        spvCode = utils::loadGlslFromFile(path);
    }

    VkShaderModuleCreateInfo createInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spvCode.size() * sizeof(spvCode[0]),
        .pCode    = spvCode.data(),
    };

    VkShaderModule handle;
    VK_CHECK_RESULT(vkCreateShaderModule(pDevice->getHandle(), &createInfo, nullptr, &handle));

    auto instance = std::unique_ptr<Shader>(new Shader(std::move(spvCode), handle, entrypoint));
    return instance;
}

Shader::Shader(std::vector<uint32_t> code, VkShaderModule shaderModule, std::string entrypoint,
               const ResourceLayout* pLayout) :
    m_entrypoint(std::move(entrypoint)),
    m_code(std::move(code))
{
    getHandle() = shaderModule;
    m_layout    = pLayout ? *pLayout : ReflectLayout(m_code);
}

ResourceLayout Shader::ReflectLayout(const std::vector<uint32_t>& code)
{
    using namespace spirv_cross;
    Compiler compiler{code};
    auto     resources = compiler.get_shader_resources();

    ResourceLayout layout;
    for(const auto& res : resources.stage_inputs)
    {
        auto location = compiler.get_decoration(res.id, spv::DecorationLocation);
        layout.inputMask |= 1u << location;
    }
    for(const auto& res : resources.stage_outputs)
    {
        auto location = compiler.get_decoration(res.id, spv::DecorationLocation);
        layout.outputMask |= 1u << location;
    }
    for(const auto& res : resources.uniform_buffers)
    {
        uint32_t set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        uint32_t binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.setShaderLayouts[set].uniformBufferMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for(const auto& res : resources.storage_buffers)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.setShaderLayouts[set].storageBufferMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }
    for(const auto& res : resources.storage_images)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        auto& type = compiler.get_type(res.type_id);
        if(type.image.dim == spv::DimBuffer)
            layout.setShaderLayouts[set].storageTexelBufferMask |= 1u << binding;
        else
            layout.setShaderLayouts[set].storageImageMask |= 1u << binding;

        if(compiler.get_type(type.image.type).basetype == SPIRType::BaseType::Float)
            layout.setShaderLayouts[set].fpMask |= 1u << binding;

        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.sampled_images)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        auto& type = compiler.get_type(res.type_id);
        if(compiler.get_type(type.image.type).basetype == SPIRType::BaseType::Float)
            layout.setShaderLayouts[set].fpMask |= 1u << binding;
        if(type.image.dim == spv::DimBuffer)
            layout.setShaderLayouts[set].sampledTexelBufferMask |= 1u << binding;
        else
            layout.setShaderLayouts[set].sampledImageMask |= 1u << binding;
        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.separate_images)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        auto& type = compiler.get_type(res.type_id);
        if(compiler.get_type(type.image.type).basetype == SPIRType::BaseType::Float)
            layout.setShaderLayouts[set].fpMask |= 1u << binding;
        if(type.image.dim == spv::DimBuffer)
            layout.setShaderLayouts[set].sampledTexelBufferMask |= 1u << binding;
        else
            layout.setShaderLayouts[set].separateImageMask |= 1u << binding;
        updateArrayInfo(layout, type, set, binding);
    }
    for(const auto& res : resources.separate_samplers)
    {
        unsigned set     = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        unsigned binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        APH_ASSERT(set < VULKAN_NUM_DESCRIPTOR_SETS);
        APH_ASSERT(binding < VULKAN_NUM_BINDINGS);

        layout.setShaderLayouts[set].samplerMask |= 1u << binding;
        updateArrayInfo(layout, compiler.get_type(res.type_id), set, binding);
    }

    if(!resources.push_constant_buffers.empty())
    {
        layout.pushConstantSize =
            compiler.get_declared_struct_size(compiler.get_type(resources.push_constant_buffers.front().base_type_id));
    }

    auto spec_constants = compiler.get_specialization_constants();
    for(auto& c : spec_constants)
    {
        if(c.constant_id >= VULKAN_NUM_TOTAL_SPEC_CONSTANTS)
        {
            VK_LOG_ERR("Spec constant ID: %u is out of range, will be ignored.", c.constant_id);
            continue;
        }

        layout.specConstantMask |= 1u << c.constant_id;
    }

    return layout;
}

void ShaderProgram::combineLayout(const ImmutableSamplerBank* samplerBank)
{
    CombinedResourceLayout programLayout{};
    if(m_shaders.count(ShaderStage::VS))
    {
        programLayout.attributeMask = m_shaders[ShaderStage::VS]->m_layout.inputMask;
    }
    if(m_shaders.count(ShaderStage::FS) && m_shaders[ShaderStage::FS])
    {
        programLayout.renderTargetMask = m_shaders[ShaderStage::FS]->m_layout.outputMask;
    }

    ImmutableSamplerBank extImmutableSamplers = {};

    for(const auto& [stage, shader] : m_shaders)
    {
        if(!shader)
        {
            continue;
        }
        auto&    shaderLayout = shader->m_layout;
        uint32_t stage_mask   = utils::VkCast(stage);

        for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            programLayout.setShaderLayouts[i].sampledImageMask |= shaderLayout.setShaderLayouts[i].sampledImageMask;
            programLayout.setShaderLayouts[i].storageImageMask |= shaderLayout.setShaderLayouts[i].storageImageMask;
            programLayout.setShaderLayouts[i].uniformBufferMask |= shaderLayout.setShaderLayouts[i].uniformBufferMask;
            programLayout.setShaderLayouts[i].storageBufferMask |= shaderLayout.setShaderLayouts[i].storageBufferMask;
            programLayout.setShaderLayouts[i].sampledTexelBufferMask |=
                shaderLayout.setShaderLayouts[i].sampledTexelBufferMask;
            programLayout.setShaderLayouts[i].storageTexelBufferMask |=
                shaderLayout.setShaderLayouts[i].storageTexelBufferMask;
            programLayout.setShaderLayouts[i].inputAttachmentMask |=
                shaderLayout.setShaderLayouts[i].inputAttachmentMask;
            programLayout.setShaderLayouts[i].samplerMask |= shaderLayout.setShaderLayouts[i].samplerMask;
            programLayout.setShaderLayouts[i].separateImageMask |= shaderLayout.setShaderLayouts[i].separateImageMask;
            programLayout.setShaderLayouts[i].fpMask |= shaderLayout.setShaderLayouts[i].fpMask;

            uint32_t active_binds =
                shaderLayout.setShaderLayouts[i].sampledImageMask | shaderLayout.setShaderLayouts[i].storageImageMask |
                shaderLayout.setShaderLayouts[i].uniformBufferMask |
                shaderLayout.setShaderLayouts[i].storageBufferMask |
                shaderLayout.setShaderLayouts[i].sampledTexelBufferMask |
                shaderLayout.setShaderLayouts[i].storageTexelBufferMask |
                shaderLayout.setShaderLayouts[i].inputAttachmentMask | shaderLayout.setShaderLayouts[i].samplerMask |
                shaderLayout.setShaderLayouts[i].separateImageMask;

            if(active_binds)
                programLayout.stagesForSets[i] |= stage_mask;

            aph::utils::forEachBit(active_binds, [&](uint32_t bit) {
                programLayout.stagesForBindings[i][bit] |= stage_mask;

                auto& combinedSize = programLayout.setShaderLayouts[i].arraySize[bit];
                auto& shaderSize   = shaderLayout.setShaderLayouts[i].arraySize[bit];
                if(combinedSize && combinedSize != shaderSize)
                    VK_LOG_ERR("Mismatch between array sizes in different shaders.");
                else
                    combinedSize = shaderSize;
            });
        }

        // Merge push constant ranges into one range.
        // Do not try to split into multiple ranges as it just complicates things for no obvious gain.
        if(shaderLayout.pushConstantSize != 0)
        {
            programLayout.pushConstantRange.stageFlags |= stage_mask;
            programLayout.pushConstantRange.size =
                std::max(programLayout.pushConstantRange.size, shaderLayout.pushConstantSize);
        }

        programLayout.specConstantMask[stage] = shaderLayout.specConstantMask;
        programLayout.combinedSpecConstantMask |= shaderLayout.specConstantMask;
        programLayout.bindlessDescriptorSetMask |= shaderLayout.bindlessSetMask;
    }

    if(samplerBank)
    {
        for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
        {
            aph::utils::forEachBit(
                programLayout.setShaderLayouts[i].samplerMask | programLayout.setShaderLayouts[i].sampledImageMask,
                [&](uint32_t binding) {
                    if(samplerBank->samplers[i][binding])
                    {
                        extImmutableSamplers.samplers[i][binding] = samplerBank->samplers[i][binding];
                        programLayout.setShaderLayouts[i].immutableSamplerMask |= 1u << binding;
                    }
                });
        }
    }

    for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
    {
        if(programLayout.stagesForSets[i] != 0)
        {
            programLayout.descriptorSetMask |= 1u << i;

            for(unsigned binding = 0; binding < VULKAN_NUM_BINDINGS; binding++)
            {
                auto& arraySize = programLayout.setShaderLayouts[i].arraySize[binding];
                if(arraySize == ShaderLayout::UNSIZED_ARRAY)
                {
                    for(unsigned i = 1; i < VULKAN_NUM_BINDINGS; i++)
                    {
                        if(programLayout.stagesForBindings[i][i] != 0)
                        {
                            VK_LOG_ERR("Using bindless for set = %u, but binding = %u has a descriptor attached to it.", i, i);
                        }
                    }

                    // Allows us to have one unified descriptor set layout for bindless.
                    programLayout.stagesForBindings[i][binding] = VK_SHADER_STAGE_ALL;
                }
                else if(arraySize == 0)
                {
                    arraySize = 1;
                }
                else
                {
                    for(unsigned i = 1; i < arraySize; i++)
                    {
                        if(programLayout.stagesForBindings[i][binding + i] != 0)
                        {
                            VK_LOG_ERR(
                                "Detected binding aliasing for (%u, %u). Binding array with %u elements starting "
                                "at (%u, "
                                "%u) overlaps.\n",
                                i, binding + i, arraySize, i, binding);
                        }
                    }
                }
            }
        }
    }

    // TODO
    m_combineLayout = std::move(programLayout);
}

void ShaderProgram::createPipelineLayout(const ImmutableSamplerBank* samplerBank)
{
    m_pSetLayouts.resize(VULKAN_NUM_DESCRIPTOR_SETS);
    unsigned num_sets = 0;
    for(unsigned i = 0; i < VULKAN_NUM_DESCRIPTOR_SETS; i++)
    {
        m_pSetLayouts[i] =
            createDescriptorSetLayout(m_pDevice, m_combineLayout.setShaderLayouts[i], samplerBank->samplers[i],
                                      m_combineLayout.stagesForBindings[i], m_poolSize);
        if(m_combineLayout.descriptorSetMask & (1u << i))
        {
            num_sets = i + 1;
        }
    }

    if(num_sets > m_pDevice->getPhysicalDevice()->getProperties().limits.maxBoundDescriptorSets)
    {
        VK_LOG_ERR("Number of sets %u exceeds device limit of %u.", num_sets,
                   m_pDevice->getPhysicalDevice()->getProperties().limits.maxBoundDescriptorSets);
    }

    VkPipelineLayoutCreateInfo info = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    if(num_sets)
    {
        info.setLayoutCount = num_sets;
        info.pSetLayouts    = m_pSetLayouts.data();
    }

    if(m_combineLayout.pushConstantRange.stageFlags != 0)
    {
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges    = &m_combineLayout.pushConstantRange;
    }

#ifdef APH_DEBUG
    VK_LOG_ERR("Creating pipeline layout.");
#endif

    auto table = m_pDevice->getDeviceTable();
    if(table->vkCreatePipelineLayout(m_pDevice->getHandle(), &info, nullptr, &m_pipeLayout) != VK_SUCCESS)
        VK_LOG_ERR("Failed to create pipeline layout.");

    createUpdateTemplates();
}

void ShaderProgram::createUpdateTemplates()
{
}
}  // namespace aph::vk
