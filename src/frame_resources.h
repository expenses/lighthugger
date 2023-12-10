#pragma once
#include "allocations/base.h"
#include "allocations/image_with_view.h"
#include "allocations/staging.h"
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

    RaiiTracyCtx(tracy::VkCtx* inner_);

    ~RaiiTracyCtx();

    RaiiTracyCtx(RaiiTracyCtx&& other);

    RaiiTracyCtx& operator=(RaiiTracyCtx&& other);
};

template<class T>
struct FlipFlipResource {
    std::array<T, 2> items;
    bool flipped;

    FlipFlipResource(T a, T b) :
        items({std::move(a), std::move(b)}),
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
    RaiiTracyCtx tracy_ctx;
};

FrameCommandData create_frame_command_data(
    const vk::raii::Device& device,
    const vk::raii::PhysicalDevice& phys_device,
    const vk::raii::Queue& queue,
    uint32_t graphics_queue_family
);

struct UploadingBuffer {
    PersistentlyMappedBuffer staging;
    AllocatedBuffer buffer;

    UploadingBuffer(
        size_t size,
        vk::BufferUsageFlags usage,
        const std::string& name,
        vma::Allocator allocator
    ) :
        staging(PersistentlyMappedBuffer(AllocatedBuffer(
            vk::BufferCreateInfo {
                .size = size,
                .usage = vk::BufferUsageFlagBits::eTransferSrc},
            {
                .flags = vma::AllocationCreateFlagBits::eMapped
                    | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
                .usage = vma::MemoryUsage::eAuto,
            },
            allocator,
            "staging " + name
        ))),
        buffer(AllocatedBuffer(
            vk::BufferCreateInfo {
                .size = size,
                .usage = usage | vk::BufferUsageFlagBits::eTransferDst},
            {
                .usage = vma::MemoryUsage::eAuto,
            },
            allocator,
            name
        )) {}

    void flush(const vk::raii::CommandBuffer& command_buffer, size_t size) {
        command_buffer.copyBuffer(
            staging.buffer.buffer,
            buffer.buffer,
            {vk::BufferCopy {.srcOffset = 0, .dstOffset = 0, .size = size}}
        );
    }
};
