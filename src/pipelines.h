
struct Pipelines {
    vk::raii::Pipeline display_transform;
    vk::raii::PipelineLayout pipeline_layout;
    vk::raii::DescriptorSetLayout texture_sampler_dsl;

    static Pipelines compile_pipelines(const vk::raii::Device& device, vk::Format swapchain_format);
};
