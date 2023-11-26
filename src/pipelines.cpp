#include "pipelines.h"

#include "shared_cpu_gpu.h"

const auto RGBA_MASK = vk::ColorComponentFlagBits::eR
    | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    | vk::ColorComponentFlagBits::eA;

const auto FILL_RASTERIZATION = vk::PipelineRasterizationStateCreateInfo {
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
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

const auto EMPTY_BLEND_STATE = vk::PipelineColorBlendStateCreateInfo {};

const auto SINGLE_REPLACE_BLEND_STATE = vk::PipelineColorBlendStateCreateInfo {
    .logicOpEnable = false,
    .attachmentCount = SINGLE_REPLACE_BLEND_ATTACHMENT.size(),
    .pAttachments = SINGLE_REPLACE_BLEND_ATTACHMENT.data()};

const auto DEPTH_WRITE_GREATER = vk::PipelineDepthStencilStateCreateInfo {
    .depthTestEnable = true,
    .depthWriteEnable = true,
    .depthCompareOp = vk::CompareOp::eGreater,
};

const auto DEPTH_WRITE_LESS = vk::PipelineDepthStencilStateCreateInfo {
    .depthTestEnable = true,
    .depthWriteEnable = true,
    .depthCompareOp = vk::CompareOp::eLess,
};

std::vector<uint8_t> read_file_to_bytes(const char* filepath) {
    std::ifstream file_stream(filepath, std::ios::binary);

    if (!file_stream) {
        dbg(filepath);
        abort();
    }

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
        .pCode = reinterpret_cast<uint32_t*>(bytes.data()),
    });

    return shader;
}

vk::raii::Pipeline name_pipeline(
    vk::raii::Pipeline pipeline,
    const vk::raii::Device& device,
    const char* name
) {
    std::string pipeline_name = std::string("pipeline ") + name;
    VkPipeline c_pipeline = *pipeline;
    device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
        .objectType = vk::ObjectType::ePipeline,
        .objectHandle = reinterpret_cast<uint64_t>(c_pipeline),
        .pObjectName = pipeline_name.data()});
    return pipeline;
}

DescriptorSetLayouts
create_descriptor_set_layouts(const vk::raii::Device& device) {
    auto everything_bindings = std::array {
        // Bindless images
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 512,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eFragment,
        },
        // instance buffer
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eVertex
                | vk::ShaderStageFlagBits::eFragment,
        },
        // Uniforms
        vk::DescriptorSetLayoutBinding {
            .binding = 2,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex
                | vk::ShaderStageFlagBits::eCompute,
        },
        // hdr framebuffer
        vk::DescriptorSetLayoutBinding {
            .binding = 3,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // clamp sampler
        vk::DescriptorSetLayoutBinding {
            .binding = 4,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // display transform LUT
        vk::DescriptorSetLayoutBinding {
            .binding = 5,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // repeat sampler
        vk::DescriptorSetLayoutBinding {
            .binding = 6,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eFragment,
        },
        // depthbuffer
        vk::DescriptorSetLayoutBinding {
            .binding = 7,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // misc storage (depth info etc.) buffer
        vk::DescriptorSetLayoutBinding {
            .binding = 8,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eVertex,
        },
        // shadow map
        vk::DescriptorSetLayoutBinding {
            .binding = 9,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // Shadowmap comparison sampler
        vk::DescriptorSetLayoutBinding {
            .binding = 10,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // draw calls buffer
        vk::DescriptorSetLayoutBinding {
            .binding = 11,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // rw scene referred framebuffer
        vk::DescriptorSetLayoutBinding {
            .binding = 12,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        vk::DescriptorSetLayoutBinding {
            .binding = 13,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
    };

    std::vector<vk::DescriptorBindingFlags> flags(everything_bindings.size());
    // Set the images as being partially bound, so not all slots have to be used.
    flags[0] = vk::DescriptorBindingFlagBits::ePartiallyBound;

    auto flags_create_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo {
        .bindingCount = static_cast<uint32_t>(flags.size()),
        .pBindingFlags = flags.data()};

    auto swapchain_storage_image_bindings = std::array {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute},
    };

    auto calc_bounding_sphere_bindings = std::array {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute},
    };

    return DescriptorSetLayouts {
        .everything = device.createDescriptorSetLayout({
            .pNext = &flags_create_info,
            .bindingCount = everything_bindings.size(),
            .pBindings = everything_bindings.data(),
        }),
        .swapchain_storage_image = device.createDescriptorSetLayout({
            .bindingCount = swapchain_storage_image_bindings.size(),
            .pBindings = swapchain_storage_image_bindings.data(),
        }),
        .calc_bounding_sphere = device.createDescriptorSetLayout({
            .bindingCount = calc_bounding_sphere_bindings.size(),
            .pBindings = calc_bounding_sphere_bindings.data(),
        }),
    };
}

Pipelines Pipelines::compile_pipelines(const vk::raii::Device& device) {
    auto descriptor_set_layouts = create_descriptor_set_layouts(device);

    auto descriptor_set_layout_array = std::array {
        *descriptor_set_layouts.everything,
        *descriptor_set_layouts.swapchain_storage_image};

    // Simple push constant for instructing the shadow pass which shadowmap to render to.
    auto push_constants = std::array {
        vk::PushConstantRange {
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .offset = 0,
            .size = sizeof(ShadowPassConstant)},
        vk::PushConstantRange {
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .offset = 0,
            .size = sizeof(DisplayTransformConstant)}};

    auto pipeline_layout =
        device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
            .setLayoutCount = descriptor_set_layout_array.size(),
            .pSetLayouts = descriptor_set_layout_array.data(),
            .pushConstantRangeCount = push_constants.size(),
            .pPushConstantRanges = push_constants.data(),
        });

    auto calc_bounding_sphere_layout_array =
        std::array {*descriptor_set_layouts.calc_bounding_sphere};

    auto calc_bounding_sphere_pipeline_layout =
        device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
            .setLayoutCount = calc_bounding_sphere_layout_array.size(),
            .pSetLayouts = calc_bounding_sphere_layout_array.data(),

        });

    auto copy_quantized_positions_push_constants =
        std::array {vk::PushConstantRange {
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
            .offset = 0,
            .size = sizeof(CopyQuantizedPositionsConstant)}};

    auto copy_quantized_positions_pipeline_layout =
        device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
            .pushConstantRangeCount =
                copy_quantized_positions_push_constants.size(),
            .pPushConstantRanges =
                copy_quantized_positions_push_constants.data(),

        });

    auto render_geometry =
        create_shader_from_file(device, "compiled_shaders/render_geometry.spv");

    auto display_transform = create_shader_from_file(
        device,
        "compiled_shaders/display_transform.spv"
    );

    auto read_depth =
        create_shader_from_file(device, "compiled_shaders/read_depth.spv");

    auto write_draw_calls = create_shader_from_file(
        device,
        "compiled_shaders/write_draw_calls.spv"
    );

    auto shadows =
        create_shader_from_file(device, "compiled_shaders/shadows.spv");

    auto calc_bounding_sphere = create_shader_from_file(
        device,
        "compiled_shaders/calc_bounding_sphere.spv"
    );

    auto copy_quantized_positions = create_shader_from_file(
        device,
        "compiled_shaders/copy_quantized_positions.spv"
    );

    auto alpha_clip =
        create_shader_from_file(device, "compiled_shaders/alpha_clip.spv");

    auto visbuffer_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *render_geometry,
            .pName = "vertex",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *render_geometry,
            .pName = "pixel"}};

    auto visbuffer_alpha_clip_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *alpha_clip,
            .pName = "vertex",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *alpha_clip,
            .pName = "pixel"}};

    auto shadow_pass_stage = std::array {vk::PipelineShaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shadows,
        .pName = "vertex",
    }};

    auto u32 = vk::Format::eR32Uint;

    auto u32_format_rendering_info = vk::PipelineRenderingCreateInfoKHR {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &u32,
        .depthAttachmentFormat = vk::Format::eD32Sfloat};

    auto depth_only_rendering_info = vk::PipelineRenderingCreateInfoKHR {
        .depthAttachmentFormat = vk::Format::eD32Sfloat};

    auto graphics_pipeline_infos = std::array {
        // Shadow pass
        vk::GraphicsPipelineCreateInfo {
            .pNext = &depth_only_rendering_info,
            .stageCount = shadow_pass_stage.size(),
            .pStages = shadow_pass_stage.data(),
            .pVertexInputState = &EMPTY_VERTEX_INPUT,
            .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
            .pViewportState = &DEFAULT_VIEWPORT_STATE,
            .pRasterizationState = &FILL_RASTERIZATION,
            .pMultisampleState = &NO_MULTISAMPLING,
            .pDepthStencilState = &DEPTH_WRITE_LESS,
            .pColorBlendState = &EMPTY_BLEND_STATE,
            .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
            .layout = *pipeline_layout,
        },
        // visibility buffer pass
        vk::GraphicsPipelineCreateInfo {
            .pNext = &u32_format_rendering_info,
            .stageCount = visbuffer_stages.size(),
            .pStages = visbuffer_stages.data(),
            .pVertexInputState = &EMPTY_VERTEX_INPUT,
            .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
            .pViewportState = &DEFAULT_VIEWPORT_STATE,
            .pRasterizationState = &FILL_RASTERIZATION,
            .pMultisampleState = &NO_MULTISAMPLING,
            .pDepthStencilState = &DEPTH_WRITE_GREATER,
            .pColorBlendState = &SINGLE_REPLACE_BLEND_STATE,
            .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
            .layout = *pipeline_layout,
        },
        // visibility alpha clip buffer pass
        vk::GraphicsPipelineCreateInfo {
            .pNext = &u32_format_rendering_info,
            .stageCount = visbuffer_alpha_clip_stages.size(),
            .pStages = visbuffer_alpha_clip_stages.data(),
            .pVertexInputState = &EMPTY_VERTEX_INPUT,
            .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
            .pViewportState = &DEFAULT_VIEWPORT_STATE,
            .pRasterizationState = &FILL_RASTERIZATION,
            .pMultisampleState = &NO_MULTISAMPLING,
            .pDepthStencilState = &DEPTH_WRITE_GREATER,
            .pColorBlendState = &SINGLE_REPLACE_BLEND_STATE,
            .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
            .layout = *pipeline_layout,
        },
    };

    auto compute_pipeline_infos = std::array {
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *read_depth,
                    .pName = "read_depth",
                },
            .layout = *pipeline_layout},
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *read_depth,
                    .pName = "generate_matrices",
                },
            .layout = *pipeline_layout},
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *write_draw_calls,
                    .pName = "write_draw_calls",
                },
            .layout = *pipeline_layout},
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *display_transform,
                    .pName = "display_transform",
                },
            .layout = *pipeline_layout},
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *render_geometry,
                    .pName = "render_geometry",
                },
            .layout = *pipeline_layout},
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *calc_bounding_sphere,
                    .pName = "calc_bounding_sphere",
                },
            .layout = *calc_bounding_sphere_pipeline_layout},
        vk::ComputePipelineCreateInfo {
            .stage =
                vk::PipelineShaderStageCreateInfo {
                    .stage = vk::ShaderStageFlagBits::eCompute,
                    .module = *copy_quantized_positions,
                    .pName = "copy_quantized_positions",
                },
            .layout = *copy_quantized_positions_pipeline_layout}};

    auto graphics_pipelines =
        device.createGraphicsPipelines(nullptr, graphics_pipeline_infos);

    auto compute_pipelines =
        device.createComputePipelines(nullptr, compute_pipeline_infos);

    return Pipelines {
        .shadow_pass = name_pipeline(
            std::move(graphics_pipelines[0]),
            device,
            "shadow_pass"
        ),
        .read_depth = name_pipeline(
            std::move(compute_pipelines[0]),
            device,
            "read_depth"
        ),
        .generate_matrices = name_pipeline(
            std::move(compute_pipelines[1]),
            device,
            "generate_matrices"
        ),
        .write_draw_calls = name_pipeline(
            std::move(compute_pipelines[2]),
            device,
            "write_draw_calls"
        ),
        .display_transform = name_pipeline(
            std::move(compute_pipelines[3]),
            device,
            "display_transform_compute"
        ),
        .write_visbuffer = std::move(graphics_pipelines[1]),
        .render_geometry = std::move(compute_pipelines[4]),
        .write_visbuffer_alphaclip = std::move(graphics_pipelines[2]),
        .pipeline_layout = std::move(pipeline_layout),
        .calc_bounding_sphere =
            {.pipeline = name_pipeline(
                 std::move(compute_pipelines[5]),
                 device,
                 "calc_bounding_sphere"
             ),
             .layout = std::move(calc_bounding_sphere_pipeline_layout)},
        .copy_quantized_positions =
            {.pipeline = name_pipeline(
                 std::move(compute_pipelines[6]),
                 device,
                 "copy_quantized_positions"
             ),
             .layout = std::move(copy_quantized_positions_pipeline_layout)},
        .dsl = std::move(descriptor_set_layouts),
    };
}
