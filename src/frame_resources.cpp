#include "frame_resources.h"

FrameCommandData create_frame_command_data(
    const vk::raii::Device& device,
    const vk::raii::PhysicalDevice& phys_device,
    const vk::raii::Queue& queue,
    uint32_t graphics_queue_family
) {
    auto pool = device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_queue_family,
    });

    auto buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {
        .commandPool = *pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1});

    auto buffer = std::move(buffers[0]);

    auto tracy_ctx = TracyVkContext(*phys_device, *device, *queue, *buffer);

    return {
        .pool = std::move(pool),
        .buffer = std::move(buffer),
        .swapchain_semaphore = device.createSemaphore({}),
        .render_semaphore = device.createSemaphore({}),
        .render_fence =
            device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled}),
        .tracy_ctx = RaiiTracyCtx(tracy_ctx)};
}


RaiiTracyCtx::RaiiTracyCtx(tracy::VkCtx* inner_) : inner(inner_) {}

RaiiTracyCtx::~RaiiTracyCtx() {
    if (inner) {
        TracyVkDestroy(inner);
    }
}

RaiiTracyCtx::RaiiTracyCtx(RaiiTracyCtx&& other) {
    std::swap(inner, other.inner);
}

RaiiTracyCtx& RaiiTracyCtx::operator=(RaiiTracyCtx&& other) {
    std::swap(inner, other.inner);
    return *this;
}
