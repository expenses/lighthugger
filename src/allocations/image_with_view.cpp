#include "image_with_view.h"

ImageWithView ImageWithView::create_image_with_view(
    vk::ImageCreateInfo create_info,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const char* name,
    vk::ImageSubresourceRange subresource_range,
    vk::ImageViewType view_type
) {
    auto image = AllocatedImage(create_info, allocator, name);
    auto view = device.createImageView(
        {.image = image.image,
         .viewType = view_type,
         .format = create_info.format,
         .subresourceRange = subresource_range}
    );
    return ImageWithView(std::move(image), std::move(view));
}
