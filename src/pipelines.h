#pragma once

struct DescriptorSetLayouts {
    vk::raii::DescriptorSetLayout everything;
};

struct Pipelines {
    vk::raii::Pipeline display_transform;
    vk::raii::Pipeline render_geometry;
    vk::raii::Pipeline geometry_depth_prepass;
    vk::raii::Pipeline shadow_pass;
    vk::raii::Pipeline read_depth;
    vk::raii::Pipeline generate_matrices;
    vk::raii::Pipeline write_draw_calls;
    vk::raii::PipelineLayout pipeline_layout;

    DescriptorSetLayouts dsl;

    static Pipelines compile_pipelines(
        const vk::raii::Device& device,
        vk::Format swapchain_format
    );
};
