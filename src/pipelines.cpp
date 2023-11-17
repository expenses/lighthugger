#include "pipelines.h"

const auto RGBA_MASK = vk::ColorComponentFlagBits::eR
    | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    | vk::ColorComponentFlagBits::eA;

const auto FILL_RASTERIZATION = vk::PipelineRasterizationStateCreateInfo {
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eNone,
    .lineWidth = 1.0f};

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
    .pDynamicStates = DEFAULT_DYNAMIC_STATES.data()};

const auto DEFAULT_VIEWPORT_STATE = vk::PipelineViewportStateCreateInfo {
    .viewportCount = 1,
    .scissorCount = 1,
};

const auto TRIANGLE_LIST_INPUT_ASSEMBLY =
    vk::PipelineInputAssemblyStateCreateInfo {
        .topology = vk::PrimitiveTopology::eTriangleList};

const vk::PipelineVertexInputStateCreateInfo EMPTY_VERTEX_INPUT = {};

const auto SINGLE_REPLACE_BLEND_ATTACHMENT =
    std::array {vk::PipelineColorBlendAttachmentState {
        .blendEnable = false,
        .colorWriteMask = RGBA_MASK}};

const auto SINGLE_REPLACE_BLEND_STATE = vk::PipelineColorBlendStateCreateInfo {
    .logicOpEnable = false,
    .attachmentCount = SINGLE_REPLACE_BLEND_ATTACHMENT.size(),
    .pAttachments = SINGLE_REPLACE_BLEND_ATTACHMENT.data()};

std::vector<uint8_t> read_file_to_bytes(const char* filepath) {
    std::ifstream file_stream(filepath, std::ios::binary);

    assert(file_stream);

    std::vector<uint8_t> contents(
        (std::istreambuf_iterator<char>(file_stream)),
        {}
    );

    assert(!contents.empty());

    return contents;
}

vk::raii::ShaderModule
create_shader_from_file(const vk::raii::Device& device, const char* filepath) {
    auto bytes = read_file_to_bytes(filepath);

    auto shader = device.createShaderModule(vk::ShaderModuleCreateInfo {
        .codeSize = bytes.size(),
        .pCode = (uint32_t*)bytes.data(),
    });

    return shader;
}

DescriptorSetLayouts
create_descriptor_set_layouts(const vk::raii::Device& device) {
    auto geometry_bindings =
        std::array {// Vertices
                    vk::DescriptorSetLayoutBinding {
                        .binding = 0,
                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                        .descriptorCount = 1,
                        .stageFlags = vk::ShaderStageFlagBits::eVertex,
                    },
                    // Indices
                    vk::DescriptorSetLayoutBinding {
                        .binding = 1,
                        .descriptorType = vk::DescriptorType::eStorageBuffer,
                        .descriptorCount = 1,
                        .stageFlags = vk::ShaderStageFlagBits::eVertex,
                    },
                    // Uniforms
                    vk::DescriptorSetLayoutBinding {
                        .binding = 2,
                        .descriptorType = vk::DescriptorType::eUniformBuffer,
                        .descriptorCount = 1,
                        .stageFlags = vk::ShaderStageFlagBits::eVertex,
                    }};

    auto display_transform_bindings =
        std::array {// scene-referred framebuffer
                    vk::DescriptorSetLayoutBinding {
                        .binding = 0,
                        .descriptorType = vk::DescriptorType::eSampledImage,
                        .descriptorCount = 1,
                        .stageFlags = vk::ShaderStageFlagBits::eFragment,
                    },
                    // sampler
                    vk::DescriptorSetLayoutBinding {
                        .binding = 1,
                        .descriptorType = vk::DescriptorType::eSampler,
                        .descriptorCount = 1,
                        .stageFlags = vk::ShaderStageFlagBits::eFragment,
                    },
                    // display transform LUT
                    vk::DescriptorSetLayoutBinding {
                        .binding = 2,
                        .descriptorType = vk::DescriptorType::eSampledImage,
                        .descriptorCount = 1,
                        .stageFlags = vk::ShaderStageFlagBits::eFragment,
                    }};

    return DescriptorSetLayouts {
        .display_transform = device.createDescriptorSetLayout({
            .bindingCount = display_transform_bindings.size(),
            .pBindings = display_transform_bindings.data(),
        }),
        .geometry = device.createDescriptorSetLayout({
            .bindingCount = geometry_bindings.size(),
            .pBindings = geometry_bindings.data(),
        }),
    };
}

Pipelines Pipelines::compile_pipelines(
    const vk::raii::Device& device,
    vk::Format swapchain_format
) {
    auto descriptor_set_layouts = create_descriptor_set_layouts(device);

    auto descriptor_set_layout_array = std::array {
        *descriptor_set_layouts.display_transform,
        *descriptor_set_layouts.geometry};

    auto pipeline_layout =
        device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
            .setLayoutCount = descriptor_set_layout_array.size(),
            .pSetLayouts = descriptor_set_layout_array.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        });

    auto clear_pretty =
        create_shader_from_file(device, "compiled_shaders/clear_pretty.spv");

    auto fullscreen_tri =
        create_shader_from_file(device, "compiled_shaders/fullscreen_tri.spv");

    auto display_transform = create_shader_from_file(
        device,
        "compiled_shaders/display_transform.spv"
    );

    auto blit_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *fullscreen_tri,
            .pName = "VSMain",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *display_transform,
            .pName = "PSMain"}};

    auto clear_pretty_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *clear_pretty,
            .pName = "VSMain",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *clear_pretty,
            .pName = "PSMain"}};

    auto swapchain_format_rendering_info = vk::PipelineRenderingCreateInfoKHR {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchain_format};

    auto rgba16f = vk::Format::eR16G16B16A16Sfloat;

    auto rgba16f_format_rendering_info = vk::PipelineRenderingCreateInfoKHR {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &rgba16f};

    auto binding_desc = vk::VertexInputBindingDescription {
        .binding = 0,
        .stride = 4 * 3,
        .inputRate = vk::VertexInputRate::eVertex
    };

    auto attr_desc = vk::VertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = 0,
    };

    auto input_state = vk::PipelineVertexInputStateCreateInfo {
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &binding_desc,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &attr_desc
    };

    auto pipeline_infos =
        std::array {// display_transform
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
                    },
                    // display_transform
                    vk::GraphicsPipelineCreateInfo {
                        .pNext = &rgba16f_format_rendering_info,
                        .stageCount = clear_pretty_stages.size(),
                        .pStages = clear_pretty_stages.data(),
                        .pVertexInputState = &EMPTY_VERTEX_INPUT,
                        .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
                        .pViewportState = &DEFAULT_VIEWPORT_STATE,
                        .pRasterizationState = &FILL_RASTERIZATION,
                        .pMultisampleState = &NO_MULTISAMPLING,
                        .pColorBlendState = &SINGLE_REPLACE_BLEND_STATE,
                        .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
                        .layout = *pipeline_layout,
                    }};

    auto pipelines = device.createGraphicsPipelines(nullptr, pipeline_infos);

    return Pipelines {
        .display_transform = std::move(pipelines[0]),
        .clear_pretty = std::move(pipelines[1]),
        .pipeline_layout = std::move(pipeline_layout),
        .dsl = std::move(descriptor_set_layouts),
    };
}
