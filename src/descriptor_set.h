#pragma once
#include "allocations/image_with_view.h"
#include "allocations/persistently_mapped.h"
#include "frame_resources.h"
#include "shared_cpu_gpu.h"

struct DescriptorSetLayouts {
    vk::raii::DescriptorSetLayout everything;
    vk::raii::DescriptorSetLayout swapchain_storage_image;
};

DescriptorSetLayouts
create_descriptor_set_layouts(const vk::raii::Device& device);

struct IndexTracker {
    uint32_t next_index = 0;
    std::vector<uint32_t> free_indices;

    IndexTracker();

    uint32_t push();

    void free(uint32_t index);

    ~IndexTracker();
};

struct DescriptorSet {
    vk::raii::DescriptorSet set;
    std::vector<vk::raii::DescriptorSet> swapchain_image_sets;
    std::shared_ptr<IndexTracker> tracker = std::make_shared<IndexTracker>();

    DescriptorSet(
        vk::raii::DescriptorSet set_,
        std::vector<vk::raii::DescriptorSet> swapchain_image_sets_
    );

    uint32_t write_image(const ImageWithView& image, vk::Device device);

    void write_resizing_descriptors(
        const ResizingResources& resizing_resources,
        const vk::raii::Device& device,
        const std::vector<vk::raii::ImageView>& swapchain_image_views
    );

    void write_descriptors(
        const Resources& resources,
        const vk::raii::Device& device,
        const std::vector<vk::raii::ImageView>& swapchain_image_views
    );
};
