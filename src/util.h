#pragma once

const vk::ImageSubresourceRange COLOR_SUBRESOURCE_RANGE = {
    .aspectMask = vk::ImageAspectFlagBits::eColor,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1,
};

void check_vk_result(vk::Result err);

float deg_to_rad(float deg);
