#include "descriptor_set.h"

uint32_t
DescriptorSet::write_image(const ImageWithView& image, vk::Device device) {
    auto index = tracker.push();

    auto image_info = vk::DescriptorImageInfo {
        .imageView = *image.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    device.updateDescriptorSets(
        {vk::WriteDescriptorSet {
            .dstSet = *set,
            .dstBinding = 0,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_info}},
        {}
    );

    return index;
}

void DescriptorSet::write_resizing_descriptors(
    const ResizingResources& resizing_resources,
    const vk::raii::Device& device
) {
    auto image_info = vk::DescriptorImageInfo {
        .imageView = *resizing_resources.scene_referred_framebuffer.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto depthbuffer_image_info = vk::DescriptorImageInfo {
        .imageView = *resizing_resources.depthbuffer.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 3,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &image_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 7,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &depthbuffer_image_info},
        },
        {}
    );
}

void DescriptorSet::write_descriptors(
    const Resources& resources,
    const vk::raii::Device& device
) {
    write_resizing_descriptors(resources.resizing, device);

    auto lut_image_info = vk::DescriptorImageInfo {
        .imageView = *resources.display_transform_lut.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto shadowmap_image_info = vk::DescriptorImageInfo {
        .imageView = *resources.shadowmap.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto clamp_sampler_info =
        vk::DescriptorImageInfo {.sampler = *resources.clamp_sampler};
    auto repeat_sampler_info =
        vk::DescriptorImageInfo {.sampler = *resources.repeat_sampler};
    auto shadowmap_comparison_sampler_info = vk::DescriptorImageInfo {
        .sampler = *resources.shadowmap_comparison_sampler};

    auto geometry_buffer_info = vk::DescriptorBufferInfo {
        .buffer = resources.geometry_buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    auto uniform_buffer_info = vk::DescriptorBufferInfo {
        .buffer = resources.uniform_buffer.buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    auto depth_info_buffer_info = vk::DescriptorBufferInfo {
        .buffer = resources.depth_info_buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    auto draw_calls_buffer_info = vk::DescriptorBufferInfo {
        .buffer = resources.draw_calls_buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    auto instance_buffer_info = vk::DescriptorBufferInfo {
        .buffer = resources.instance_buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    auto draw_counts_buffer = vk::DescriptorBufferInfo {
        .buffer = resources.draw_counts_buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    // Write initial descriptor sets.
    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &geometry_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 2,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &uniform_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 4,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &clamp_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 5,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &lut_image_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 6,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &repeat_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 8,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &depth_info_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 9,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &shadowmap_image_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 10,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &shadowmap_comparison_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 11,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &draw_calls_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 12,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &instance_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 13,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &draw_counts_buffer},
        },
        {}
    );
}
