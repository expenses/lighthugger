#pragma once
#include "util.h"

struct ResizingResources {
    ImageWithView scene_referred_framebuffer;
    ImageWithView depthbuffer;

    ResizingResources(
        const vk::raii::Device& device,
        vma::Allocator allocator,
        vk::Extent2D extent
    ) :
        scene_referred_framebuffer(ImageWithView(
            {
                .imageType = vk::ImageType::e2D,
                .format = vk::Format::eR16G16B16A16Sfloat,
                .extent =
                    vk::Extent3D {
                        .width = extent.width,
                        .height = extent.height,
                        .depth = 1,
                    },
                .mipLevels = 1,
                .arrayLayers = 1,
                .usage = vk::ImageUsageFlagBits::eColorAttachment
                    | vk::ImageUsageFlagBits::eSampled,
            },
            allocator,
            device,
            "scene_referred_framebuffer",
            COLOR_SUBRESOURCE_RANGE
        )),
        depthbuffer(ImageWithView(
            {.imageType = vk::ImageType::e2D,
             .format = vk::Format::eD32Sfloat,
             .extent =
                 vk::Extent3D {
                     .width = extent.width,
                     .height = extent.height,
                     .depth = 1,
                 },
             .mipLevels = 1,
             .arrayLayers = 1,
             .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment
                 | vk::ImageUsageFlagBits::eSampled},
            allocator,
            device,
            "depthbuffer",
            DEPTH_SUBRESOURCE_RANGE
        )) {}
};

struct Resources {
    ResizingResources resizing;
    PersistentlyMappedBuffer uniform_buffer;
    ImageWithView shadowmap;
    AllocatedBuffer depth_info_buffer;
    AllocatedBuffer instance_buffer;
    AllocatedBuffer draw_calls_buffer;
    AllocatedBuffer draw_counts_buffer;
    uint32_t max_num_draws;
    std::array<vk::raii::ImageView, 4> shadowmap_layer_views;
    ImageWithView display_transform_lut;
    vk::raii::Sampler repeat_sampler;
    vk::raii::Sampler clamp_sampler;
    vk::raii::Sampler shadowmap_comparison_sampler;
};
