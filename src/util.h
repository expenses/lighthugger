#pragma once

#include "allocations.h"

const vk::ImageSubresourceRange COLOR_SUBRESOURCE_RANGE = {
    .aspectMask = vk::ImageAspectFlagBits::eColor,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1,
};

const vk::ImageSubresourceRange DEPTH_SUBRESOURCE_RANGE = {
    .aspectMask = vk::ImageAspectFlagBits::eDepth,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1,
};

void check_vk_result(vk::Result err);


std::vector<vk::raii::ImageView> create_swapchain_image_views(
    const vk::raii::Device& device,
    const std::vector<vk::Image>& swapchain_images,
    vk::Format swapchain_format
);

struct PersistentlyMappedBuffer {
    AllocatedBuffer buffer;
    void* mapped_ptr;

    PersistentlyMappedBuffer(AllocatedBuffer buffer_) :
        buffer(std::move(buffer_)) {
        auto buffer_info =
            buffer.allocator.getAllocationInfo(buffer.allocation);
        mapped_ptr = buffer_info.pMappedData;
        assert(mapped_ptr);
    }
};
