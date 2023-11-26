#pragma once

struct DescriptorSetLayouts {
    vk::raii::DescriptorSetLayout everything;
    vk::raii::DescriptorSetLayout swapchain_storage_image;
    vk::raii::DescriptorSetLayout calc_bounding_sphere;
};

struct PipelineAndLayout {
    vk::raii::Pipeline pipeline;
    vk::raii::PipelineLayout layout;
};

struct Pipelines {
    vk::raii::Pipeline shadow_pass;
    vk::raii::Pipeline read_depth;
    vk::raii::Pipeline generate_matrices;
    vk::raii::Pipeline write_draw_calls;
    vk::raii::Pipeline display_transform;
    vk::raii::Pipeline write_visbuffer;
    vk::raii::Pipeline render_geometry;
    vk::raii::Pipeline write_visbuffer_alphaclip;
    vk::raii::PipelineLayout pipeline_layout;

    PipelineAndLayout calc_bounding_sphere;
    PipelineAndLayout copy_quantized_positions;

    DescriptorSetLayouts dsl;

    static Pipelines compile_pipelines(const vk::raii::Device& device);
};
