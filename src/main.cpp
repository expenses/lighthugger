#include "pch.h"

// Sources:
// https://vkguide.dev
// https://www.glfw.org/docs/latest/vulkan_guide.html
// https://vulkan-tutorial.com
// https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md
// https://github.com/KhronosGroup/Vulkan-Hpp
// https://lesleylai.info/en/vk-khr-dynamic-rendering/

// Make inserting color image transition barriers easier.

struct ImageBarrier {
    ThsvsAccessType prev_access;
    ThsvsAccessType next_access;
    ThsvsImageLayout prev_layout;
    ThsvsImageLayout next_layout;
    bool discard_contents;
    uint32_t queue_family;
    vk::Image image;
};

void insert_color_image_barrier(vk::CommandBuffer command_buffer, ImageBarrier barrier) {
    ThsvsImageBarrier image_barrier = {
            .prevAccessCount = 1,
            .pPrevAccesses = &barrier.prev_access,
            .nextAccessCount = 1,
            .pNextAccesses = &barrier.next_access,
            .prevLayout = barrier.prev_layout,
            .nextLayout = barrier.next_layout,
            .discardContents = barrier.discard_contents,
            .srcQueueFamilyIndex = barrier.queue_family,
            .dstQueueFamilyIndex = barrier.queue_family,
            .image = barrier.image,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };
    thsvsCmdPipelineBarrier(command_buffer, nullptr, 0, nullptr, 1, &image_barrier);
}

struct AllocatedImage {
    vk::Image image;
    vma::Allocation allocation;
};

AllocatedImage create_image(vk::ImageCreateInfo create_info, vma::Allocator allocator) {
    AllocatedImage image;
    vma::AllocationCreateInfo alloc_info = {.usage = vma::MemoryUsage::eAuto};
    auto err = allocator.createImage(&create_info, &alloc_info, &image.image, &image.allocation, nullptr);
    return image;
}

int main() {
    glfwInit();

    // Most of this stuff is copied right from VkBootstrap
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name ("Example Vulkan Application")
                        .request_validation_layers ()
                        .use_default_debug_messenger ()
                        .build ();
    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
        return -1;
    }
    vkb::Instance vkb_inst = inst_ret.value();

    auto instance = vkb_inst.instance;

    vk::Extent2D extent = {
        .width = 640,
        .height = 480,
    };

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(extent.width, extent.height, "Window Title", NULL, NULL);

    VkSurfaceKHR _surface;
    vk::Result err = static_cast<vk::Result>(glfwCreateWindowSurface(instance, window, NULL, &_surface));
    vk::SurfaceKHR surface = vk::SurfaceKHR(_surface);

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    auto phys_device = selector.set_surface (surface)
                        // Require the dynamic rendering extension
                        .add_required_extension("VK_KHR_dynamic_rendering")
                        // Dependencies of the above
                        .add_required_extension("VK_KHR_depth_stencil_resolve")
                        .add_required_extension("VK_KHR_create_renderpass2")
                        .add_required_extension("VK_KHR_multiview")
                        .add_required_extension("VK_KHR_maintenance2")
                        .set_minimum_version (1, 2)
                        .select ()
                        .value ();

    vk::PhysicalDeviceDynamicRenderingFeatures dyn_rendering_features = {
        .dynamicRendering = true
    };

    vkb::DeviceBuilder device_builder{ phys_device };
    auto vkb_device = device_builder.add_pNext(&dyn_rendering_features).build().value();

    vk::Device device = vkb_device.device;

    // Init the default dispatch loader for device extension functions etc.
    // If we don't do this `command_buffer.beginRendering` etc. will segfault as it can't find the function.
    vk::DispatchLoaderDynamic dldi = vk::DispatchLoaderDynamic( instance, vkGetInstanceProcAddr );

    vk::DispatchLoaderDynamic dldd = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr, device);

    vk::Queue graphics_queue = vkb_device.get_queue (vkb::QueueType::graphics).value();
    auto graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    vkb::SwapchainBuilder swapchain_builder{ phys_device, vkb_device, surface };
    auto swapchain = swapchain_builder
        .use_default_format_selection()
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(extent.width, extent.height)
        .build ()
        .value();

    auto swapchain_images = swapchain.get_images().value();
    auto swapchain_image_views = swapchain.get_image_views().value();

    auto command_pool = device.createCommandPool({
        .queueFamilyIndex = graphics_queue_family,
    });

    // AMD VMA allocator

    vma::AllocatorCreateInfo allocatorCreateInfo = {
        .physicalDevice = phys_device.physical_device,
        .device = device,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    vma::Allocator allocator;
    auto vma_err = vma::createAllocator(&allocatorCreateInfo, &allocator);


    auto img2 = device.createImage({
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR16G16B16A16Sfloat,
        .extent = vk::Extent3D {
            .width = extent.width,
            .height = extent.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
    });

    vk::ImageMemoryRequirementsInfo2 req =  {
        .image = img2
    };
    VkMemoryRequirements2 xyzzzzz;
    //device.getImageMemoryRequirements2(req, dldd);

    std::cout << "!!" << std::endl;

    /*auto scene_referred_framebuffer = create_image({
        .imageType = vk::ImageType::e2D,
        .format = vk::Format::eR16G16B16A16Sfloat,
        .extent = vk::Extent3D {
            .width = extent.width,
            .height = extent.height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
    }, allocator);*/



    /*auto scene_referred_framebuffer_view = device.createImageView({
        .image = scene_referred_framebuffer,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR16G16B16A16Sfloat,
    });*/

    /*
    vma::AllocatorInfo info;

    info.instance = instance;
    info.physicalDevice = phys_device.physical_device;
    info.device = device;
    vk::ResultTypeValue<vma::Allocator> allocator = vma::createAllocator(info);
    */

    std::vector<vk::CommandBuffer> command_buffers = device.allocateCommandBuffers({
        .commandPool = command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    });
    vk::CommandBuffer command_buffer = command_buffers[0];

    vk::Semaphore present_semaphore = device.createSemaphore({});
    vk::Semaphore render_semaphore = device.createSemaphore({});
    vk::Fence render_fence = device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});

    float time = 0.0;

    while (!glfwWindowShouldClose(window)) {
        int current_width, current_height;
        glfwGetWindowSize(window, &current_width, &current_height);
        vk::Extent2D current_extent = {
            .width = static_cast<uint32_t>(current_width),
            .height = static_cast<uint32_t>(current_height)
        };
        if (extent != current_extent) {
            printf("Resizing from %ux%u to %ux%u\n", extent.width, extent.height, current_extent.width, current_extent.height);
            extent = current_extent;

            swapchain = swapchain_builder
                .set_desired_extent(extent.width, extent.height)
                .build ()
                .value();

            swapchain_images = swapchain.get_images().value();
            swapchain_image_views = swapchain.get_image_views().value();
        }

        time += 1.0 / 60.0;

        glfwPollEvents();

        auto u64_max = std::numeric_limits<uint64_t>::max();
        // Wait on the render fence to be signaled
        // (it's signaled before this loop starts so that we don't just block forever on the first frame)
        auto err = device.waitForFences(1, &render_fence, true, u64_max);
        err = device.resetFences(1, &render_fence);
        // Reset the command pool instead of resetting the single command buffer as
        // it's cheaper (afaik). Obviously don't do this if multiple command buffers are used.
        device.resetCommandPool(command_pool);

        // Acquire the next swapchain image (waiting on the gpu-side and signaling the present semaphore when finished).
        auto swapchain_image_index = device.acquireNextImageKHR(vk::SwapchainKHR(swapchain), u64_max, present_semaphore, nullptr).value;

        // This wraps vkBeginCommandBuffer.
        command_buffer.begin({
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        });

        // Transition the swapchain image from whatever it was before
        // to being used as a color attachment, discarding contents in the process.
        insert_color_image_barrier(command_buffer, {
            .prev_access = THSVS_ACCESS_NONE,
            .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
            .prev_layout = THSVS_IMAGE_LAYOUT_OPTIMAL,
            .next_layout = THSVS_IMAGE_LAYOUT_OPTIMAL,
            .discard_contents = true,
            .queue_family = graphics_queue_family,
            .image = swapchain_images[swapchain_image_index]
        });

        // Setup a clear color.
        std::array<float, 4> clear_color = {fabsf(sinf(time)), 0.1, 0.1, 1.0};
        vk::RenderingAttachmentInfoKHR color_attachment_info = {
            .imageView = swapchain_image_views[swapchain_image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {
                .color = {
                    .float32 = clear_color,
                },
            }
        };
        // This just clears the swapchain image with the clear color.
        // Wraps vkCmdBeginRendering
        command_buffer.beginRendering({
            .renderArea = {
                .offset = {},
                .extent = extent,
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment_info
        }, dldd);
        command_buffer.endRendering(dldd);

        // Transition the swapchain image from being used as a color attachment
        // to presenting. Don't discard contents!!
        insert_color_image_barrier(command_buffer, {
            .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
            .next_access = THSVS_ACCESS_PRESENT,
            .prev_layout = THSVS_IMAGE_LAYOUT_OPTIMAL,
            .next_layout = THSVS_IMAGE_LAYOUT_OPTIMAL,
            .discard_contents = false,
            .queue_family = graphics_queue_family,
            .image = swapchain_images[swapchain_image_index]
        });

        command_buffer.end();

        vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
        // Submit the command buffer, waiting on the present semaphore and
        // signaling the render semaphore when the commands are finished.
        vk::SubmitInfo submit_info = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &present_semaphore,
            .pWaitDstStageMask = &dst_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_semaphore,
        };
        // This wraps vkQueueSubmit.
        err = graphics_queue.submit(1, &submit_info, render_fence);

        auto cpp_swapchain = vk::SwapchainKHR(swapchain);
        // Present the swapchain image after having wated on the render semaphore.
        // This wraps vkQueuePresentKHR.
        err = graphics_queue.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &cpp_swapchain,
            .pImageIndices = &swapchain_image_index,
        });
    }

    return 0;
}