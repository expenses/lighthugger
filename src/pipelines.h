struct DescriptorSetLayouts {
    vk::raii::DescriptorSetLayout display_transform;
    vk::raii::DescriptorSetLayout geometry;
};

struct Pipelines {
    vk::raii::Pipeline display_transform;
    vk::raii::Pipeline render_geometry;
    vk::raii::Pipeline geometry_depth_prepass;
    vk::raii::PipelineLayout pipeline_layout;

    DescriptorSetLayouts dsl;

    static Pipelines compile_pipelines(
        const vk::raii::Device& device,
        vk::Format swapchain_format
    );
};
