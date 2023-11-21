#pragma once
#include "allocations/image_with_view.h"
#include "allocations/persistently_mapped.h"
#include "frame_resources.h"
#include "shared_cpu_gpu.h"

struct IndexTracker {
    uint32_t next_index = 0;
    std::vector<uint32_t> free_indices;

    IndexTracker() {}

    uint32_t push() {
        if (!free_indices.empty()) {
            uint32_t index = free_indices.back();
            free_indices.pop_back();
            return index;
        }

        uint32_t index = next_index;
        next_index += 1;

        return index;
    }

    void free(uint32_t index) {
        free_indices.push_back(index);
    }
};

struct DescriptorSet {
    vk::raii::DescriptorSet set;
    IndexTracker tracker;

    DescriptorSet(vk::raii::DescriptorSet set_) : set(std::move(set_)) {}

    uint32_t write_image(const ImageWithView& image, vk::Device device);

    void write_resizing_descriptors(
        const ResizingResources& resizing_resources,
        const vk::raii::Device& device
    );

    void write_descriptors(
        const Resources& resources,
        const vk::raii::Device& device
    );
};
