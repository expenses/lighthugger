#pragma once
#include "descriptor_set.h"

struct PipelineAndLayout {
    vk::raii::Pipeline pipeline;
    vk::raii::PipelineLayout layout;
};

struct RasterizationPipeline {
    vk::raii::Pipeline opaque;
    vk::raii::Pipeline alpha_clip;
};

struct Pipelines {
    RasterizationPipeline rasterize_shadowmap;
    RasterizationPipeline rasterize_visbuffer;

    vk::raii::Pipeline read_depth;
    vk::raii::Pipeline generate_matrices;
    vk::raii::Pipeline write_draw_calls;
    vk::raii::Pipeline display_transform;
    vk::raii::Pipeline render_geometry;
    vk::raii::Pipeline reset_buffers_a;
    vk::raii::Pipeline reset_buffers_b;
    vk::raii::Pipeline reset_buffers_c;
    vk::raii::Pipeline write_draw_calls_shadows;
    vk::raii::Pipeline cull_instances;
    vk::raii::Pipeline cull_instances_shadows;

    vk::raii::PipelineLayout pipeline_layout;

    PipelineAndLayout copy_quantized_positions;

    static Pipelines compile_pipelines(
        const vk::raii::Device& device,
        const DescriptorSetLayouts& descriptor_set_layouts
    );
};
