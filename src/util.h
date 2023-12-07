#pragma once

#include "allocations/base.h"

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

std::vector<vk::raii::ImageView> create_and_name_swapchain_image_views(
    const vk::raii::Device& device,
    const std::vector<vk::Image>& swapchain_images,
    vk::Format swapchain_format
);

vk::DescriptorBufferInfo buffer_info(const AllocatedBuffer& buffer);

uint32_t dispatch_size(uint32_t width, uint32_t workgroup_size);

std::vector<uint8_t> read_file_to_bytes(const std::filesystem::path& filepath);
