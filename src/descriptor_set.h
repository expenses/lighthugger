#pragma once
#include "util.h"

struct BindlessImageTracker {
    uint32_t next_image_index = 0;
    std::vector<uint32_t> free_indices;

    BindlessImageTracker() {}

    uint32_t push() {
        if (!free_indices.empty()) {
            uint32_t index = free_indices.back();
            free_indices.pop_back();
            return index;
        }

        uint32_t index = next_image_index;
        next_image_index += 1;

        return index;
    }

    void free(uint32_t index) {
        free_indices.push_back(index);
    }
};

struct DescriptorSet {
    vk::raii::DescriptorSet set;
    BindlessImageTracker tracker;

    DescriptorSet(vk::raii::DescriptorSet set_) : set(std::move(set_)) {}

    uint32_t write_image(const ImageWithView& image, vk::Device device) {
        auto index = tracker.push();

        auto image_info = vk::DescriptorImageInfo {
            .imageView = *image.view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        device.updateDescriptorSets(
            {vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 6,
                .dstArrayElement = index,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &image_info}},
            {}
        );

        return index;
    }
};
