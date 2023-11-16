#include "pch.h"
#include "allocations.h"
#include "sync.h"
#include "debugging.h"
#include "pipelines.h"
#include "image_loading.h"

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

std::vector<vk::raii::ImageView> create_swapchain_image_views(const vk::raii::Device& device, const std::vector<vk::Image>& swapchain_images, vk::Format swapchain_format) {
    std::vector<vk::raii::ImageView> views;

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

    vk::raii::Context context;

    vk::raii::Instance instance(context, vk::InstanceCreateInfo{
        .flags = {}, .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()), .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()), .ppEnabledExtensionNames = instance_extensions.data() });

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance, vkGetInstanceProcAddr);

    std::optional<vk::raii::DebugUtilsMessengerEXT> messenger = std::nullopt;

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
    std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
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

    vk::raii::Device device = phys_device.createDevice({
        .pNext = &dyn_rendering_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    }, nullptr);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance, vkGetInstanceProcAddr, *device);

    auto graphics_queue = device.getQueue(graphics_queue_family, 0);

    vk::Extent2D extent = {
        .width = 640,
        .height = 480,
    };

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(extent.width, extent.height, "Window Title", NULL, NULL);

    glfwSetKeyCallback(window, key_callback);

    VkSurfaceKHR _surface;
    vk::Result err = static_cast<vk::Result>(glfwCreateWindowSurface(*instance, window, NULL, &_surface));
    vk::raii::SurfaceKHR surface = vk::raii::SurfaceKHR(instance, _surface);

    // Todo: get minimagecount and imageformat properly.
    vk::SwapchainCreateInfoKHR swapchain_create_info = {
        .surface = *surface,
        .minImageCount = 3,
        .imageFormat = vk::Format::eB8G8R8A8Srgb,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .presentMode = vk::PresentModeKHR::eFifo,
    };
    auto swapchain = device.createSwapchainKHR(swapchain_create_info);

    auto swapchain_images = swapchain.getImages();
    auto swapchain_image_views = create_swapchain_image_views(device, swapchain_images, swapchain_create_info.imageFormat);

    vk::raii::CommandPool command_pool = device.createCommandPool({
        .queueFamilyIndex = graphics_queue_family,
    });

    // AMD VMA allocator

    vma::AllocatorCreateInfo allocatorCreateInfo = {
        .physicalDevice = *phys_device,
        .device = *device,
        .instance = *instance,
        .vulkanApiVersion = vulkan_version,
    };

    vma::Allocator allocator;
    auto vma_err = vma::createAllocator(&allocatorCreateInfo, &allocator);

    auto scene_referred_framebuffer = AllocatedImage({
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
    }, allocator, "scene_referred_framebuffer");

    auto scene_referred_framebuffer_view = device.createImageView({
            .image = scene_referred_framebuffer.image,
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR16G16B16A16Sfloat,
            .subresourceRange = COLOR_SUBRESOURCE_RANGE
        });

    auto command_buffers = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {
        .commandPool = *command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    });
    auto command_buffer = std::move(command_buffers[0]);

    auto present_semaphore = device.createSemaphore({});
    auto render_semaphore = device.createSemaphore({});
    auto render_fence = device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});

    float time = 0.0;

    auto pipelines = Pipelines::compile_pipelines(device, swapchain_create_info.imageFormat);

    std::array<vk::DescriptorPoolSize, 2> pool_sizes = {
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1024
        },
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eSampler,
            .descriptorCount = 1024
        }
    };

    auto descriptor_pool = device.createDescriptorPool({
        .maxSets = 128,
        .poolSizeCount = pool_sizes.size(),
        .pPoolSizes = pool_sizes.data()
    });

    std::array<vk::DescriptorSetLayout, 1> descriptor_set_layouts = {
        *pipelines.texture_sampler_dsl
    };

    auto descriptor_sets = device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {
        .descriptorPool = *descriptor_pool,
        .descriptorSetCount = descriptor_set_layouts.size(),
        .pSetLayouts = descriptor_set_layouts.data()
    });

    auto scene_referred_framebuffer_ds = std::move(descriptor_sets[0]);

    auto sampler = device.createSampler({

    });

    auto init_fence = device.createFence({});

    command_buffer.begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });

    // Load all resources
    auto display_transform_lut_loaded = load_dds("tony-mc-mapface/shader/tony_mc_mapface.dds", allocator, command_buffer, graphics_queue_family);

    command_buffer.end();

    vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;

    vk::SubmitInfo submit_info = {
        .pWaitDstStageMask = &dst_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*command_buffer,
    };
    graphics_queue.submit(submit_info, *init_fence);

    auto u64_max = std::numeric_limits<uint64_t>::max();

    auto init_err = device.waitForFences({*init_fence}, true, u64_max);

    // Drop staging buffer.
    {auto _ = std::move(display_transform_lut_loaded.staging_buffer);}

    auto display_transform_lut = std::move(display_transform_lut_loaded.image);
    auto display_transform_view = device.createImageView({
            .image = display_transform_lut.image,
            .viewType = vk::ImageViewType::e3D,
            .format = vk::Format::eE5B9G9R9UfloatPack32,
            .subresourceRange = COLOR_SUBRESOURCE_RANGE
        });

    auto image_info = vk::DescriptorImageInfo {
        .imageView = *scene_referred_framebuffer_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    auto lut_image_info = vk::DescriptorImageInfo {
        .imageView = *display_transform_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    auto sampler_info = vk::DescriptorImageInfo {
        .sampler = *sampler
    };


    // Write initial descriptor sets.
    device.updateDescriptorSets({
        vk::WriteDescriptorSet {
            .dstSet = *scene_referred_framebuffer_ds,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_info
        },
        vk::WriteDescriptorSet {
            .dstSet = *scene_referred_framebuffer_ds,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampler,
            .pImageInfo = &sampler_info
        },
        vk::WriteDescriptorSet {
            .dstSet = *scene_referred_framebuffer_ds,
            .dstBinding = 2,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &lut_image_info
        }
    }, {});

    while (!glfwWindowShouldClose(window)) {
        int current_width, current_height;
        glfwGetWindowSize(window, &current_width, &current_height);
        vk::Extent2D current_extent = {
            .width = static_cast<uint32_t>(current_width),
            .height = static_cast<uint32_t>(current_height)
        };
        if (extent != current_extent) {
            graphics_queue.waitIdle();

            extent = current_extent;

            swapchain_create_info.imageExtent = extent;
            swapchain_create_info.oldSwapchain = *swapchain;

            // todo: this prints a validation error on resize. Still works fine though.
            // ideally https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_swapchain_maintenance1.html is used to avoid this.
            swapchain = device.createSwapchainKHR(swapchain_create_info);

            swapchain_images = swapchain.getImages();
            swapchain_image_views = create_swapchain_image_views(device, swapchain_images, swapchain_create_info.imageFormat);

            scene_referred_framebuffer = AllocatedImage({
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
            }, allocator, "scene_referred_framebuffer");

            scene_referred_framebuffer_view = device.createImageView({
                    .image = scene_referred_framebuffer.image,
                    .viewType = vk::ImageViewType::e2D,
                    .format = vk::Format::eR16G16B16A16Sfloat,
                    .subresourceRange = COLOR_SUBRESOURCE_RANGE
                });

            image_info = vk::DescriptorImageInfo {
                .imageView = *scene_referred_framebuffer_view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };

            device.updateDescriptorSets({
                vk::WriteDescriptorSet {
                    .dstSet = *scene_referred_framebuffer_ds,
                    .dstBinding = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eSampledImage,
                    .pImageInfo = &image_info
                }
            }, {});
        }

        time += 1.0 / 60.0;

        glfwPollEvents();

        // Wait on the render fence to be signaled
        // (it's signaled before this loop starts so that we don't just block forever on the first frame)
        auto err = device.waitForFences({*render_fence}, true, u64_max);
        device.resetFences({*render_fence});
        // Reset the command pool instead of resetting the single command buffer as
        // it's cheaper (afaik). Obviously don't do this if multiple command buffers are used.
        command_pool.reset();

        // Acquire the next swapchain image (waiting on the gpu-side and signaling the present semaphore when finished).
        auto [acquire_err, swapchain_image_index] = swapchain.acquireNextImage(u64_max, *present_semaphore);

        // This wraps vkBeginCommandBuffer.
        command_buffer.begin({
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        });

        command_buffer.setScissor(0, {
            vk::Rect2D {
                .offset = {},
                .extent = extent
            }
        });
        command_buffer.setViewport(0, {
            vk::Viewport {
                .width = extent.width,
                .height = extent.height,
                .minDepth = 0.0,
                .maxDepth = 1.0
            }
        });

        insert_color_image_barriers(command_buffer, std::array{ImageBarrier{
            .prev_access = THSVS_ACCESS_NONE,
            .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
            .discard_contents = true,
            .queue_family = graphics_queue_family,
            .image = scene_referred_framebuffer.image
        }});

        // Setup a clear color.
        std::array<float, 4> clear_color = {fabsf(sinf(time)), 0.1, 0.1, 1.0};

        vk::RenderingAttachmentInfoKHR framebuffer_attachment_info = {
            .imageView = *scene_referred_framebuffer_view,
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

        insert_color_image_barriers(command_buffer, std::array {
            ImageBarrier {
                .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                .next_access = THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                .discard_contents = false,
                .queue_family = graphics_queue_family,
                .image = scene_referred_framebuffer.image
            },
            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = swapchain_images[swapchain_image_index]
            }
        });

        vk::RenderingAttachmentInfoKHR color_attachment_info = {
            .imageView = *swapchain_image_views[swapchain_image_index],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {}
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

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines.display_transform);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipelines.pipeline_layout, 0, {
            *scene_referred_framebuffer_ds
        }, {});
        command_buffer.draw(3, 1, 0, 0);

        command_buffer.endRendering();

        // Transition the swapchain image from being used as a color attachment
        // to presenting. Don't discard contents!!
        insert_color_image_barriers(command_buffer, std::array{ImageBarrier{
            .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
            .next_access = THSVS_ACCESS_PRESENT,
            .discard_contents = false,
            .queue_family = graphics_queue_family,
            .image = swapchain_images[swapchain_image_index]
        }});

        command_buffer.end();

        vk::PipelineStageFlags dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
        // Submit the command buffer, waiting on the present semaphore and
        // signaling the render semaphore when the commands are finished.
        vk::SubmitInfo submit_info = {
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*present_semaphore,
            .pWaitDstStageMask = &dst_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &*render_semaphore,
        };
        // This wraps vkQueueSubmit.
        graphics_queue.submit(submit_info, *render_fence);

        // Present the swapchain image after having wated on the render semaphore.
        // This wraps vkQueuePresentKHR.
        err = graphics_queue.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swapchain,
            .pImageIndices = &swapchain_image_index,
        });
    }

    // Wait until the device is idle so that we don't get destructor warnings about currently in-use resources.
    device.waitIdle();

    // Todo: this needs to happen after all the allocated memory is destructed.
    // The allocator should be placed in a raii wrapper before any allocations are made.
    allocator.destroy();

    return 0;
}
