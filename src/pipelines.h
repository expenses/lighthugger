#include <fstream>

const auto RGBA_MASK = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

const auto FILL_RASTERIZATION = vk::PipelineRasterizationStateCreateInfo {
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eNone,
    .lineWidth = 1.0f
};

const auto NO_MULTISAMPLING = vk::PipelineMultisampleStateCreateInfo {
    .rasterizationSamples = vk::SampleCountFlagBits::e1,
    .sampleShadingEnable = false,
    .minSampleShading = 1.0f,
    .alphaToCoverageEnable = false,
    .alphaToOneEnable = false,
};

const std::array<vk::DynamicState, 2> DEFAULT_DYNAMIC_STATES = {
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor,
};

const auto DEFAULT_DYNAMIC_STATE_INFO = vk::PipelineDynamicStateCreateInfo {
    .dynamicStateCount = DEFAULT_DYNAMIC_STATES.size(),
    .pDynamicStates = DEFAULT_DYNAMIC_STATES.data()
};

const auto DEFAULT_VIEWPORT_STATE = vk::PipelineViewportStateCreateInfo {
    .viewportCount = 1,
    .scissorCount = 1,
};

const auto TRIANGLE_LIST_INPUT_ASSEMBLY = vk::PipelineInputAssemblyStateCreateInfo {
    .topology = vk::PrimitiveTopology::eTriangleList
};

const vk::PipelineVertexInputStateCreateInfo EMPTY_VERTEX_INPUT = {};

const auto SINGLE_REPLACE_BLEND_ATTACHMENT = std::array {
    vk::PipelineColorBlendAttachmentState {
        .blendEnable = false,
        .colorWriteMask = RGBA_MASK
    }
};

const auto SINGLE_REPLACE_BLEND_STATE = vk::PipelineColorBlendStateCreateInfo {
    .logicOpEnable = false,
    .attachmentCount = SINGLE_REPLACE_BLEND_ATTACHMENT.size(),
    .pAttachments = SINGLE_REPLACE_BLEND_ATTACHMENT.data()
};

std::vector<uint8_t> read_file_to_bytes(const char* filepath) {
    std::ifstream file_stream(filepath, std::ios::binary);

    assert(file_stream);

    std::vector<uint8_t> contents((std::istreambuf_iterator<char>(file_stream)), {});

    assert(!contents.empty());

    return contents;
}

vk::raii::ShaderModule create_shader_from_file(const vk::raii::Device& device, const char* filepath) {
    auto bytes = read_file_to_bytes(filepath);

    auto shader = device.createShaderModule(vk::ShaderModuleCreateInfo{
        .codeSize = bytes.size(),
        .pCode = (uint32_t*)bytes.data(),
    });

    return shader;
}

struct Pipelines {
    vk::raii::Pipeline display_transform;
    vk::raii::PipelineLayout pipeline_layout;
    vk::raii::DescriptorSetLayout texture_sampler_dsl;

    static Pipelines compile_pipelines(const vk::raii::Device& device, vk::Format swapchain_format) {
        auto blit_vs = create_shader_from_file(device, "compiled_shaders/blit_vs.spv");
        auto blit_ps = create_shader_from_file(device, "compiled_shaders/blit_ps.spv");

        std::array<vk::DescriptorSetLayoutBinding, 2> texture_sampler_bindings = {
            vk::DescriptorSetLayoutBinding {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            },
            vk::DescriptorSetLayoutBinding {
                .binding = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            }
        };

        auto texture_sampler_dsl = device.createDescriptorSetLayout({
            .bindingCount = texture_sampler_bindings.size(),
            .pBindings = texture_sampler_bindings.data(),
        });

        std::array<vk::DescriptorSetLayout, 1> descriptor_set_layouts = {
            *texture_sampler_dsl
        };

        auto pipeline_layout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
            .setLayoutCount = descriptor_set_layouts.size(),
            .pSetLayouts = descriptor_set_layouts.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        });

        std::array<vk::PipelineShaderStageCreateInfo, 2> blit_stages = {
            vk::PipelineShaderStageCreateInfo {
                .stage = vk::ShaderStageFlagBits::eVertex,
                .module = *blit_vs,
                .pName = "VSMain",
            },
            vk::PipelineShaderStageCreateInfo {
                .stage = vk::ShaderStageFlagBits::eFragment,
                .module = *blit_ps,
                .pName = "PSMain"
            }
        };

        auto swapchain_format_rendering_info = vk::PipelineRenderingCreateInfoKHR {
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapchain_format
        };

        std::array<vk::GraphicsPipelineCreateInfo, 1> pipeline_infos = {
            // display_transform
            vk::GraphicsPipelineCreateInfo {
                .pNext = &swapchain_format_rendering_info,
                .stageCount = blit_stages.size(),
                .pStages = blit_stages.data(),
                .pVertexInputState = &EMPTY_VERTEX_INPUT,
                .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
                .pViewportState = &DEFAULT_VIEWPORT_STATE,
                .pRasterizationState = &FILL_RASTERIZATION,
                .pMultisampleState = &NO_MULTISAMPLING,
                .pColorBlendState = &SINGLE_REPLACE_BLEND_STATE,
                .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
                .layout = *pipeline_layout,
            }
        };

        auto pipelines = device.createGraphicsPipelines(nullptr, pipeline_infos);

        return Pipelines {
            .display_transform = std::move(pipelines[0]),
            .pipeline_layout = std::move(pipeline_layout),
            .texture_sampler_dsl = std::move(texture_sampler_dsl),
        };
    }
};
