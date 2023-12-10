#pragma once
#include "descriptor_set.h"

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

    vk::raii::Pipeline copy_quantized_positions;
    vk::raii::Pipeline copy_quantized_normals;
    vk::raii::PipelineLayout copy_pipeline_layout;

    static Pipelines compile_pipelines(
        const vk::raii::Device& device,
        const DescriptorSetLayouts& descriptor_set_layouts
    );
};
