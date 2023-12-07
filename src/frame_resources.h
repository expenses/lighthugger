#pragma once
#include "allocations/base.h"
#include "allocations/image_with_view.h"
#include "shared_cpu_gpu.h"
#include "util.h"

struct ResizingResources {
    ImageWithView scene_referred_framebuffer;
    ImageWithView depthbuffer;
    ImageWithView visbuffer;

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
                .usage = vk::ImageUsageFlagBits::eStorage
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
        )),
        visbuffer(ImageWithView(
            {.imageType = vk::ImageType::e2D,
             .format = vk::Format::eR32Uint,
             .extent =
                 vk::Extent3D {
                     .width = extent.width,
                     .height = extent.height,
                     .depth = 1,
                 },
             .mipLevels = 1,
             .arrayLayers = 1,
             .usage = vk::ImageUsageFlagBits::eColorAttachment
                 | vk::ImageUsageFlagBits::eSampled},
            allocator,
            device,
            "visbuffer",
            COLOR_SUBRESOURCE_RANGE
        )) {}
};

struct InstanceResources {
    AllocatedBuffer instances;
    AllocatedBuffer meshlet_references;
    AllocatedBuffer num_meshlets_prefix_sum;
};

struct Resources {
    ResizingResources resizing;
    ImageWithView shadowmap;
    AllocatedBuffer misc_storage_buffer;
    AllocatedBuffer draw_calls_buffer;
    AllocatedBuffer dispatches_buffer;
    std::array<vk::raii::ImageView, 4> shadowmap_layer_views;
    ImageWithView display_transform_lut;
    ImageWithView skybox;
    vk::raii::Sampler repeat_sampler;
    vk::raii::Sampler clamp_sampler;
    vk::raii::Sampler shadowmap_comparison_sampler;
};

struct RaiiTracyCtx {
    tracy::VkCtx* inner;

    RaiiTracyCtx(tracy::VkCtx* inner_) : inner(inner_) {}

    ~RaiiTracyCtx() {
        if (inner) {
            TracyVkDestroy(inner);
        }
    }

    RaiiTracyCtx(RaiiTracyCtx&& other) {
        std::swap(inner, other.inner);
    }

    RaiiTracyCtx& operator=(RaiiTracyCtx&& other) {
        std::swap(inner, other.inner);
        return *this;
    }
};

template<class T>
struct FlipFlipResource {
    std::array<T, 2> items;
    bool flipped;

    FlipFlipResource(std::array<T, 2> items_) :
        items(std::move(items_)),
        flipped(false) {}

    void flip() {
        flipped = !flipped;
    }

    T& get() {
        return items[flipped];
    }
};

struct FrameCommandData {
    vk::raii::CommandPool pool;
    vk::raii::CommandBuffer buffer;
    vk::raii::Semaphore swapchain_semaphore;
    vk::raii::Semaphore render_semaphore;
    vk::raii::Fence render_fence;
    AllocatedBuffer uniform_buffer;
    RaiiTracyCtx tracy_ctx;
};

FrameCommandData create_frame_command_data(
    const vk::raii::Device& device,
    const vk::raii::PhysicalDevice& phys_device,
    const vk::raii::Queue& queue,
    vma::Allocator allocator,
    uint32_t graphics_queue_family
);
