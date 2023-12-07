#include "util.h"

// https://github.com/KhronosGroup/Vulkan-Hpp/blob/64539664151311b63485a42277db7ffaba1c0c63/samples/RayTracing/RayTracing.cpp#L539-L549
void check_vk_result(vk::Result err) {
    if (err != vk::Result::eSuccess) {
        dbg(vk::to_string(err));
        if (err < vk::Result::eSuccess) {
            abort();
        }
    }
}

std::vector<vk::raii::ImageView> create_and_name_swapchain_image_views(
    const vk::raii::Device& device,
    const std::vector<vk::Image>& swapchain_images,
    vk::Format swapchain_format
) {
    std::vector<vk::raii::ImageView> views;

    for (size_t i = 0; i < swapchain_images.size(); i++) {
        vk::Image image = swapchain_images[i];
        VkImage c_image = image;
        std::string name = std::string("swapchain image ") + std::to_string(i);
        device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
            .objectType = vk::ObjectType::eImage,
            .objectHandle = reinterpret_cast<uint64_t>(c_image),
            .pObjectName = name.data()});

        vk::raii::ImageView view = device.createImageView(
            {.image = image,
             .viewType = vk::ImageViewType::e2D,
             .format = swapchain_format,
             .subresourceRange = COLOR_SUBRESOURCE_RANGE}
        );

        VkImageView c_image_view = *view;
        name = std::string("swapchain image view ") + std::to_string(i);
        device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
            .objectType = vk::ObjectType::eImageView,
            .objectHandle = reinterpret_cast<uint64_t>(c_image_view),
            .pObjectName = name.data()});

        views.push_back(std::move(view));
    }

    return views;
}

vk::DescriptorBufferInfo buffer_info(const AllocatedBuffer& buffer) {
    return vk::DescriptorBufferInfo {
        .buffer = buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};
}

uint32_t dispatch_size(uint32_t width, uint32_t workgroup_size) {
    return ((width - 1) / workgroup_size) + 1;
}

std::vector<uint8_t> read_file_to_bytes(const std::filesystem::path& filepath) {
    std::ifstream stream(filepath, std::ios::binary);

    if (!stream) {
        dbg(filepath);
        abort();
    }

    stream.seekg(0, stream.end);
    auto length = stream.tellg();
    stream.seekg(0, stream.beg);

    std::vector<uint8_t> contents(length);
    stream.read((char*)contents.data(), length);

    return contents;
}
