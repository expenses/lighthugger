#pragma once
#include "base.h"

struct ImageWithView {
    AllocatedImage image;
    vk::raii::ImageView view;

    ImageWithView(AllocatedImage image_, vk::raii::ImageView view_);

    static ImageWithView create_image_with_view(
        vk::ImageCreateInfo create_info,
        vma::Allocator allocator,
        const vk::raii::Device& device,
        const std::string& name,
        vk::ImageSubresourceRange subresource_range,
        vk::ImageViewType view_type
    );

    ImageWithView(
        vk::ImageCreateInfo create_info,
        vma::Allocator allocator,
        const vk::raii::Device& device,
        const std::string& name,
        vk::ImageSubresourceRange subresource_range,
        vk::ImageViewType view_type = vk::ImageViewType::e2D
    );
};
