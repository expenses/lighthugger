#include "image_with_view.h"

ImageWithView ImageWithView::create_image_with_view(
    vk::ImageCreateInfo create_info,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const std::string& name,
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
    auto view_name = name + " view";
    VkImageView c_view = *view;
    device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
        .objectType = vk::ObjectType::eImageView,
        .objectHandle = reinterpret_cast<uint64_t>(c_view),
        .pObjectName = view_name.data()});
    return ImageWithView(std::move(image), std::move(view));
}

ImageWithView::ImageWithView(AllocatedImage image_, vk::raii::ImageView view_) :
    image(std::move(image_)),
    view(std::move(view_)) {}

ImageWithView::ImageWithView(
    vk::ImageCreateInfo create_info,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const std::string& name,
    vk::ImageSubresourceRange subresource_range,
    vk::ImageViewType view_type
) :
    ImageWithView(ImageWithView::create_image_with_view(
        create_info,
        allocator,
        device,
        name,
        subresource_range,
        view_type
    )) {}
