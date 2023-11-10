#include "pch.h"

// Sources:
// https://vkguide.dev
// https://www.glfw.org/docs/latest/vulkan_guide.html
// https://vulkan-tutorial.com
// https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md
// https://github.com/KhronosGroup/Vulkan-Hpp
// https://lesleylai.info/en/vk-khr-dynamic-rendering/
// https://github.com/dokipen3d/vulkanHppMinimalExample/blob/master/main.cpp

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

const vk::ImageSubresourceRange COLOR_SUBRESOURCE_RANGE = {
    .aspectMask = vk::ImageAspectFlagBits::eColor,
    .baseMipLevel = 0,
    .levelCount = 1,
    .baseArrayLayer = 0,
    .layerCount = 1,
};

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
        .subresourceRange = COLOR_SUBRESOURCE_RANGE
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

VKAPI_ATTR VkBool32 VKAPI_CALL debug_message_callback( VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
                                                 VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
                                                 VkDebugUtilsMessengerCallbackDataEXT const * pCallbackData,
                                                 void * /*pUserData*/ )
{
    std::cout
        << "["
        << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>( messageSeverity ))
        << "]["
        << vk::to_string( static_cast<vk::DebugUtilsMessageTypeFlagsEXT>( messageTypes ) )
        << "]["
        << pCallbackData->pMessageIdName
        << "]\t"
        << pCallbackData->pMessage
        << std::endl;

  return false;
}

std::vector<vk::ImageView> create_swapchain_image_views(vk::Device device, const std::vector<vk::Image>& swapchain_images, vk::Format swapchain_format) {
    std::vector<vk::ImageView> views;

    for (vk::Image image: swapchain_images) {
        views.push_back(device.createImageView({
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = swapchain_format,
            .subresourceRange = COLOR_SUBRESOURCE_RANGE
        }));
    }

    return views;
}

int main() {
    glfwInit();

    auto vulkan_version = VK_API_VERSION_1_2;

    vk::ApplicationInfo appInfo = {
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0), .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0), .apiVersion = vulkan_version
    };

    auto glfwExtensionCount = 0u;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> instance_extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    std::vector<const char*> layers;

    auto debug_enabled = true;

    if (debug_enabled) {
        instance_extensions.push_back("VK_EXT_debug_utils");
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    auto instance = vk::createInstance(vk::InstanceCreateInfo{
        .flags = {}, .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()), .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()), .ppEnabledExtensionNames = instance_extensions.data() });

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance, vkGetInstanceProcAddr);

    vk::DebugUtilsMessengerEXT messenger;

    if (debug_enabled) {
        messenger = instance.createDebugUtilsMessengerEXT({
            .flags = {},
            .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = debug_message_callback
        }, nullptr);
    }

    // todo: physical device and queue family selection.
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    auto phys_device = physicalDevices[0];
    auto queueFamilyProperties = phys_device.getQueueFamilyProperties();
    auto queue_fam = queueFamilyProperties[0];

    uint32_t graphics_queue_family = 0;

    float queue_prio = 1.0f;

    vk::DeviceQueueCreateInfo device_queue_create_info = {
        .queueCount = 1,
        .pQueuePriorities = &queue_prio
    };


    vk::PhysicalDeviceDynamicRenderingFeatures dyn_rendering_features = {
        .dynamicRendering = true
    };

    std::vector<const char*> device_extensions = {
        "VK_KHR_swapchain",
        "VK_KHR_dynamic_rendering"
    };

    vk::Device device = phys_device.createDevice({
        .pNext = &dyn_rendering_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    }, nullptr);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance, vkGetInstanceProcAddr, device);

    auto graphics_queue = device.getQueue(graphics_queue_family, 0);

    vk::Extent2D extent = {
        .width = 640,
        .height = 480,
    };

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(extent.width, extent.height, "Window Title", NULL, NULL);

    glfwSetKeyCallback(window, key_callback);

    VkSurfaceKHR _surface;
    vk::Result err = static_cast<vk::Result>(glfwCreateWindowSurface(instance, window, NULL, &_surface));
    vk::SurfaceKHR surface = vk::SurfaceKHR(_surface);

    // Todo: get minimagecount and imageformat properly.
    vk::SwapchainCreateInfoKHR swapchain_create_info = {
        .surface = surface,
        .minImageCount = 3,
        .imageFormat = vk::Format::eB8G8R8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .presentMode = vk::PresentModeKHR::eFifo,
    };
    auto swapchain = device.createSwapchainKHR(swapchain_create_info);

    auto swapchain_images = device.getSwapchainImagesKHR(swapchain);
    auto swapchain_image_views = create_swapchain_image_views(device, swapchain_images, swapchain_create_info.imageFormat);

    auto command_pool = device.createCommandPool({
        .queueFamilyIndex = graphics_queue_family,
    });

    // AMD VMA allocator

    vma::AllocatorCreateInfo allocatorCreateInfo = {
        .physicalDevice = phys_device,
        .device = device,
        .instance = instance,
        .vulkanApiVersion = vulkan_version,
    };

    vma::Allocator allocator;
    auto vma_err = vma::createAllocator(&allocatorCreateInfo, &allocator);

    auto scene_referred_framebuffer = create_image({
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
    }, allocator);

    auto scene_referred_framebuffer_view = device.createImageView({
            .image = scene_referred_framebuffer.image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR16G16B16A16Sfloat,
            .subresourceRange = COLOR_SUBRESOURCE_RANGE
        });

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
            extent = current_extent;

            swapchain_create_info.imageExtent = extent;
            swapchain_create_info.oldSwapchain = swapchain;

            // todo: this prints a validation error on resize. Still works fine though.
            // ideally https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_swapchain_maintenance1.html is used to avoid this.
            swapchain = device.createSwapchainKHR(swapchain_create_info);

            swapchain_images = device.getSwapchainImagesKHR(swapchain);
            swapchain_image_views = create_swapchain_image_views(device, swapchain_images, swapchain_create_info.imageFormat);

            device.destroySwapchainKHR(swapchain_create_info.oldSwapchain);

            scene_referred_framebuffer = create_image({
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
            }, allocator);

            scene_referred_framebuffer_view = device.createImageView({
                    .image = scene_referred_framebuffer.image,
                    .viewType = vk::ImageViewType::e2D,
                    .format = vk::Format::eR16G16B16A16Sfloat,
                    .subresourceRange = COLOR_SUBRESOURCE_RANGE
                });
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
        auto swapchain_image_index = device.acquireNextImageKHR(swapchain, u64_max, present_semaphore, nullptr).value;

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


        vk::RenderingAttachmentInfoKHR framebuffer_attachment_info = {
            .imageView = scene_referred_framebuffer_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {
                .color = {
                    .float32 = clear_color,
                },
            }
        };
        command_buffer.beginRendering({
            .renderArea = {
                .offset = {},
                .extent = extent,
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &framebuffer_attachment_info
        });
        command_buffer.endRendering();

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
        });
        command_buffer.endRendering();

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

        // Present the swapchain image after having wated on the render semaphore.
        // This wraps vkQueuePresentKHR.
        err = graphics_queue.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &swapchain_image_index,
        });
    }

    return 0;
}
