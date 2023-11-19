#include "util.h"

// https://github.com/KhronosGroup/Vulkan-Hpp/blob/64539664151311b63485a42277db7ffaba1c0c63/samples/RayTracing/RayTracing.cpp#L539-L549
void check_vk_result(vk::Result err) {
    if (err != vk::Result::eSuccess) {
        std::cerr << "Vulkan error " << vk::to_string(err);
        if (err < vk::Result::eSuccess) {
            abort();
        }
    }
}

std::vector<vk::raii::ImageView> create_swapchain_image_views(
    const vk::raii::Device& device,
    const std::vector<vk::Image>& swapchain_images,
    vk::Format swapchain_format
) {
    std::vector<vk::raii::ImageView> views;

    for (vk::Image image : swapchain_images) {
        views.push_back(device.createImageView(
            {.image = image,
             .viewType = vk::ImageViewType::e2D,
             .format = swapchain_format,
             .subresourceRange = COLOR_SUBRESOURCE_RANGE}
        ));
    }

    return views;
}
