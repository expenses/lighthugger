#include "pipelines.h"

#include "shared_cpu_gpu.h"
#include "util.h"

const auto RGBA_MASK = vk::ColorComponentFlagBits::eR
    | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB
    | vk::ColorComponentFlagBits::eA;

const auto FILL_RASTERIZATION = vk::PipelineRasterizationStateCreateInfo {
    .polygonMode = vk::PolygonMode::eFill,
    .cullMode = vk::CullModeFlagBits::eBack,
    .lineWidth = 1.0f};

const auto FILL_RASTERIZATION_CULL_FRONT_FACES =
    vk::PipelineRasterizationStateCreateInfo {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eFront,
        .lineWidth = 1.0f};

const auto FILL_RASTERIZATION_DOUBLE_SIDED =
    vk::PipelineRasterizationStateCreateInfo {
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

vk::raii::ShaderModule create_shader_from_file(
    const vk::raii::Device& device,
    const std::filesystem::path& filepath
) {
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
    const std::string& name
) {
    std::string pipeline_name = std::string("pipeline ") + name;
    VkPipeline c_pipeline = *pipeline;
    device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
        .objectType = vk::ObjectType::ePipeline,
        .objectHandle = reinterpret_cast<uint64_t>(c_pipeline),
        .pObjectName = pipeline_name.data()});
    return pipeline;
}

vk::raii::Pipeline create_compute_pipeline_from_shader(
    const vk::raii::Device& device,
    const vk::raii::PipelineLayout& layout,
    const std::filesystem::path& filepath
) {
    auto shader = create_shader_from_file(device, filepath);

    auto create_info = std::array {vk::ComputePipelineCreateInfo {
        .stage =
            vk::PipelineShaderStageCreateInfo {
                .stage = vk::ShaderStageFlagBits::eCompute,
                .module = *shader,
                .pName = "main",
            },
        .layout = *layout}};

    return name_pipeline(
        std::move(device.createComputePipelines(nullptr, create_info)[0]),
        device,
        filepath.string()
    );
}

Pipelines Pipelines::compile_pipelines(
    const vk::raii::Device& device,
    const DescriptorSetLayouts& descriptor_set_layouts
) {
    auto descriptor_set_layout_array = std::array {
        *descriptor_set_layouts.everything,
        *descriptor_set_layouts.swapchain_storage_image};

    // Simple push constant for instructing the shadow pass which shadowmap to render to.
    auto push_constant_ranges = std::array {vk::PushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex
            | vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(UniformBufferAddressConstant)
            + sizeof(UniformBufferAddressConstant)}};

    auto pipeline_layout =
        device.createPipelineLayout(vk::PipelineLayoutCreateInfo {
            .setLayoutCount = descriptor_set_layout_array.size(),
            .pSetLayouts = descriptor_set_layout_array.data(),
            .pushConstantRangeCount = push_constant_ranges.size(),
            .pPushConstantRanges = push_constant_ranges.data(),
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

    auto visbuffer_opaque_vertex = create_shader_from_file(
        device,
        "compiled_shaders/visbuffer_opaque_vertex.spv"
    );

    auto visbuffer_opaque_pixel = create_shader_from_file(
        device,
        "compiled_shaders/visbuffer_opaque_pixel.spv"
    );

    auto visbuffer_alpha_clip_pixel = create_shader_from_file(
        device,
        "compiled_shaders/visbuffer_alpha_clip_pixel.spv"
    );

    auto visbuffer_alpha_clip_vertex = create_shader_from_file(
        device,
        "compiled_shaders/visbuffer_alpha_clip_vertex.spv"
    );

    auto shadowmap_opaque_vertex = create_shader_from_file(
        device,
        "compiled_shaders/shadowmap_opaque_vertex.spv"
    );

    auto shadowmap_alpha_clip_vertex = create_shader_from_file(
        device,
        "compiled_shaders/shadowmap_alpha_clip_vertex.spv"
    );

    auto shadowmap_alpha_clipped_pixel = create_shader_from_file(
        device,
        "compiled_shaders/shadowmap_alpha_clipped_pixel.spv"
    );

    auto visbuffer_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *visbuffer_opaque_vertex,
            .pName = "main",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *visbuffer_opaque_pixel,
            .pName = "main"}};

    auto visbuffer_alpha_clip_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *visbuffer_alpha_clip_vertex,
            .pName = "main",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *visbuffer_alpha_clip_pixel,
            .pName = "main"}};

    auto opaque_shadow_stage = std::array {vk::PipelineShaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *shadowmap_opaque_vertex,
        .pName = "main",
    }};

    auto alpha_clip_shadow_stages = std::array {
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = *shadowmap_alpha_clip_vertex,
            .pName = "main",
        },
        vk::PipelineShaderStageCreateInfo {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *shadowmap_alpha_clipped_pixel,
            .pName = "main",
        },
    };

    auto u32 = vk::Format::eR32Uint;

    auto u32_format_rendering_info = vk::PipelineRenderingCreateInfoKHR {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &u32,
        .depthAttachmentFormat = vk::Format::eD32Sfloat};

    auto depth_only_rendering_info = vk::PipelineRenderingCreateInfoKHR {
        .depthAttachmentFormat = vk::Format::eD32Sfloat};

    auto graphics_pipeline_infos = std::array {
        // opaque shadowmaps
        vk::GraphicsPipelineCreateInfo {
            .pNext = &depth_only_rendering_info,
            .stageCount = opaque_shadow_stage.size(),
            .pStages = opaque_shadow_stage.data(),
            .pVertexInputState = &EMPTY_VERTEX_INPUT,
            .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
            .pViewportState = &DEFAULT_VIEWPORT_STATE,
            .pRasterizationState = &FILL_RASTERIZATION_CULL_FRONT_FACES,
            .pMultisampleState = &NO_MULTISAMPLING,
            .pDepthStencilState = &DEPTH_WRITE_LESS,
            .pColorBlendState = &EMPTY_BLEND_STATE,
            .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
            .layout = *pipeline_layout,
        },
        // alpha clip shadow maps
        vk::GraphicsPipelineCreateInfo {
            .pNext = &depth_only_rendering_info,
            .stageCount = alpha_clip_shadow_stages.size(),
            .pStages = alpha_clip_shadow_stages.data(),
            .pVertexInputState = &EMPTY_VERTEX_INPUT,
            .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
            .pViewportState = &DEFAULT_VIEWPORT_STATE,
            .pRasterizationState = &FILL_RASTERIZATION_DOUBLE_SIDED,
            .pMultisampleState = &NO_MULTISAMPLING,
            .pDepthStencilState = &DEPTH_WRITE_LESS,
            .pColorBlendState = &EMPTY_BLEND_STATE,
            .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
            .layout = *pipeline_layout,
        },
        // opaque visibility buffer
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
        // alpha clip visibility buffer
        vk::GraphicsPipelineCreateInfo {
            .pNext = &u32_format_rendering_info,
            .stageCount = visbuffer_alpha_clip_stages.size(),
            .pStages = visbuffer_alpha_clip_stages.data(),
            .pVertexInputState = &EMPTY_VERTEX_INPUT,
            .pInputAssemblyState = &TRIANGLE_LIST_INPUT_ASSEMBLY,
            .pViewportState = &DEFAULT_VIEWPORT_STATE,
            .pRasterizationState = &FILL_RASTERIZATION_DOUBLE_SIDED,
            .pMultisampleState = &NO_MULTISAMPLING,
            .pDepthStencilState = &DEPTH_WRITE_GREATER,
            .pColorBlendState = &SINGLE_REPLACE_BLEND_STATE,
            .pDynamicState = &DEFAULT_DYNAMIC_STATE_INFO,
            .layout = *pipeline_layout,
        }};

    auto graphics_pipelines =
        device.createGraphicsPipelines(nullptr, graphics_pipeline_infos);

    return {
        .rasterize_shadowmap {
            .opaque = name_pipeline(
                std::move(graphics_pipelines[0]),
                device,
                "rasterize_shadowmap::opaque"
            ),
            .alpha_clip = name_pipeline(
                std::move(graphics_pipelines[1]),
                device,
                "rasterize_shadowmap::alpha_clip"
            )},
        .rasterize_visbuffer =
            {.opaque = name_pipeline(
                 std::move(graphics_pipelines[2]),
                 device,
                 "rasterize_visbuffer::opaque"
             ),
             .alpha_clip = name_pipeline(
                 std::move(graphics_pipelines[3]),
                 device,
                 "rasterize_visbuffer::alpha_clip"
             )},
        .read_depth = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/compute/read_depth.spv"
        ),
        .generate_matrices = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/compute/generate_shadow_matrices.spv"
        ),
        .write_draw_calls = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/write_draw_calls.spv"
        ),
        .display_transform = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/display_transform.spv"
        ),
        .render_geometry = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/render_geometry.spv"
        ),
        .expand_meshlets = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/expand_meshlets.spv"
        ),
        .reset_buffers = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/compute/reset_buffers.spv"
        ),
        .write_draw_calls_shadows = create_compute_pipeline_from_shader(
            device,
            pipeline_layout,
            "compiled_shaders/write_draw_calls_shadows.spv"
        ),
        .pipeline_layout = std::move(pipeline_layout),
        .copy_quantized_positions =
            {.pipeline = create_compute_pipeline_from_shader(
                 device,
                 copy_quantized_positions_pipeline_layout,
                 "compiled_shaders/compute/copy_quantized_positions.spv"
             ),
             .layout = std::move(copy_quantized_positions_pipeline_layout)},
    };
}
