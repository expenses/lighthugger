#include "allocations.h"
#include "debugging.h"
#include "descriptor_set.h"
#include "pch.h"
#include "pipelines.h"
#include "projection.h"
#include "resources/image_loading.h"
#include "resources/mesh_loading.h"
#include "sync.h"

const auto u64_max = std::numeric_limits<uint64_t>::max();

// Sources:
// https://vkguide.dev
// https://www.glfw.org/docs/latest/vulkan_guide.html
// https://vulkan-tutorial.com
// https://github.com/charles-lunarg/vk-bootstrap/blob/main/docs/getting_started.md
// https://github.com/KhronosGroup/Vulkan-Hpp
// https://lesleylai.info/en/vk-khr-dynamic-rendering/
// https://github.com/dokipen3d/vulkanHppMinimalExample/blob/master/main.cpp

static void key_callback(
    GLFWwindow* window,
    int key,
    int /*scancode*/,
    int action,
    int /*mods*/
) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main() {
    glfwInit();

    auto vulkan_version = VK_API_VERSION_1_2;

    vk::ApplicationInfo appInfo = {
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .apiVersion = vulkan_version};

    auto glfwExtensionCount = 0u;
    auto glfwExtensions =
        glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> instance_extensions(
        glfwExtensions,
        glfwExtensions + glfwExtensionCount
    );
    std::vector<const char*> layers;

    auto debug_enabled = true;

    if (debug_enabled) {
        instance_extensions.push_back("VK_EXT_debug_utils");
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    vk::raii::Context context;

    vk::raii::Instance instance(
        context,
        vk::InstanceCreateInfo {
            .flags = {},
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(layers.size()),
            .ppEnabledLayerNames = layers.data(),
            .enabledExtensionCount =
                static_cast<uint32_t>(instance_extensions.size()),
            .ppEnabledExtensionNames = instance_extensions.data()}
    );

    VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance, vkGetInstanceProcAddr);

    std::optional<vk::raii::DebugUtilsMessengerEXT> messenger = std::nullopt;

    if (debug_enabled) {
        messenger = instance.createDebugUtilsMessengerEXT(
            {.flags = {},
             .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
                 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
                 | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
             .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
                 | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
             .pfnUserCallback = debug_message_callback},
            nullptr
        );
    }

    // todo: physical device and queue family selection.
    std::vector<vk::raii::PhysicalDevice> physicalDevices =
        instance.enumeratePhysicalDevices();
    auto phys_device = physicalDevices[0];
    auto queueFamilyProperties = phys_device.getQueueFamilyProperties();
    //auto queue_fam = queueFamilyProperties[0];

    uint32_t graphics_queue_family = 0;

    float queue_prio = 1.0f;

    vk::DeviceQueueCreateInfo device_queue_create_info = {
        .queueCount = 1,
        .pQueuePriorities = &queue_prio};

    auto vulkan_1_2_features = vk::PhysicalDeviceVulkan12Features {
        .descriptorBindingPartiallyBound = true,
        .runtimeDescriptorArray = true,
        .bufferDeviceAddress = true,
    };

    vk::PhysicalDeviceDynamicRenderingFeatures dyn_rendering_features = {
        .pNext = &vulkan_1_2_features,
        .dynamicRendering = true};

    auto vulkan_1_0_features = vk::PhysicalDeviceFeatures {.shaderInt64 = true};

    auto device_extensions =
        std::array {"VK_KHR_swapchain", "VK_KHR_dynamic_rendering"};

    vk::raii::Device device = phys_device.createDevice(
        {
            .pNext = &dyn_rendering_features,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &device_queue_create_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount =
                static_cast<uint32_t>(device_extensions.size()),
            .ppEnabledExtensionNames = device_extensions.data(),
            .pEnabledFeatures = &vulkan_1_0_features,
        },
        nullptr
    );

    VULKAN_HPP_DEFAULT_DISPATCHER
        .init(*instance, vkGetInstanceProcAddr, *device);

    auto graphics_queue = device.getQueue(graphics_queue_family, 0);

    vk::Extent2D extent = {
        .width = 640,
        .height = 480,
    };

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(
        extent.width,
        extent.height,
        "Window Title",
        NULL,
        NULL
    );

    glfwSetKeyCallback(window, key_callback);

    VkSurfaceKHR _surface;
    check_vk_result(static_cast<vk::Result>(
        glfwCreateWindowSurface(*instance, window, NULL, &_surface)
    ));
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
    auto swapchain_image_views = create_swapchain_image_views(
        device,
        swapchain_images,
        swapchain_create_info.imageFormat
    );

    vk::raii::CommandPool command_pool = device.createCommandPool({
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_queue_family,
    });

    // AMD VMA allocator

    vma::AllocatorCreateInfo allocatorCreateInfo = {
        .flags = vma::AllocatorCreateFlagBits::eBufferDeviceAddress,
        .physicalDevice = *phys_device,
        .device = *device,
        .instance = *instance,
        .vulkanApiVersion = vulkan_version,
    };

    vma::Allocator allocator;
    check_vk_result(vma::createAllocator(&allocatorCreateInfo, &allocator));

    auto scene_referred_framebuffer = ImageWithView(
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
    );

    auto depthbuffer = ImageWithView(
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
         .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment},
        allocator,
        device,
        "depthbuffer",
        DEPTH_SUBRESOURCE_RANGE
    );

    auto command_buffers =
        device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {
            .commandPool = *command_pool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1});
    auto command_buffer = std::move(command_buffers[0]);

    auto present_semaphore = device.createSemaphore({});
    auto render_semaphore = device.createSemaphore({});
    auto render_fence =
        device.createFence({.flags = vk::FenceCreateFlagBits::eSignaled});

    float time = 0.0;

    auto pipelines =
        Pipelines::compile_pipelines(device, swapchain_create_info.imageFormat);

    auto pool_sizes = std::array {
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1024},
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eSampler,
            .descriptorCount = 1024},
        vk::DescriptorPoolSize {
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1024}};

    auto descriptor_pool = device.createDescriptorPool(
        {.maxSets = 128,
         .poolSizeCount = pool_sizes.size(),
         .pPoolSizes = pool_sizes.data()}
    );

    auto descriptor_set_layouts = std::array {*pipelines.dsl.everything};

    auto descriptor_sets =
        device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {
            .descriptorPool = *descriptor_pool,
            .descriptorSetCount = descriptor_set_layouts.size(),
            .pSetLayouts = descriptor_set_layouts.data()});

    auto descriptor_set = DescriptorSet(std::move(descriptor_sets[0]));

    auto uniform_buffer = PersistentlyMappedBuffer(AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = sizeof(glm::mat4),
            .usage = vk::BufferUsageFlagBits::eUniformBuffer},
        {
            .flags = vma::AllocationCreateFlagBits::eMapped
                | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator
    ));

    auto clamp_sampler = device.createSampler({
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,
    });

    auto repeat_sampler = device.createSampler(
        {.magFilter = vk::Filter::eLinear,
         .minFilter = vk::Filter::eLinear,
         .addressModeU = vk::SamplerAddressMode::eRepeat,
         .addressModeV = vk::SamplerAddressMode::eRepeat,
         .addressModeW = vk::SamplerAddressMode::eRepeat,
         .maxLod = VK_LOD_CLAMP_NONE}
    );

    std::vector<AllocatedBuffer> temp_buffers;

    command_buffer.begin(
        {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit}
    );

    // Load all resources
    auto display_transform_lut = load_dds(
        "external/tony-mc-mapface/shader/tony_mc_mapface.dds",
        allocator,
        device,
        command_buffer,
        graphics_queue_family,
        temp_buffers
    );

    auto powerplant = load_obj(
        "powerplant/Powerplant.obj",
        allocator,
        device,
        command_buffer,
        graphics_queue_family,
        temp_buffers,
        descriptor_set
    );

    command_buffer.end();

    {
        vk::PipelineStageFlags dst_stage_mask =
            vk::PipelineStageFlagBits::eTransfer;

        vk::SubmitInfo submit_info = {
            .pWaitDstStageMask = &dst_stage_mask,
            .commandBufferCount = 1,
            .pCommandBuffers = &*command_buffer,
        };
        auto init_fence = device.createFence({});
        graphics_queue.submit(submit_info, *init_fence);

        check_vk_result(device.waitForFences({*init_fence}, true, u64_max));

        // Drop temp buffers.
        temp_buffers.clear();
    }

    auto buffer_addresses = std::array {powerplant.get_addresses(device)};

    auto geometry_buffer = AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = buffer_addresses.size() * sizeof(MeshBufferAddresses),
            .usage = vk::BufferUsageFlagBits::eTransferSrc
                | vk::BufferUsageFlagBits::eStorageBuffer},
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator,
        "geometry_buffer"
    );

    geometry_buffer.map_and_memcpy(
        (void*)buffer_addresses.data(),
        buffer_addresses.size() * sizeof(MeshBufferAddresses)
    );

    auto image_info = vk::DescriptorImageInfo {
        .imageView = *scene_referred_framebuffer.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto lut_image_info = vk::DescriptorImageInfo {
        .imageView = *display_transform_lut.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto clamp_sampler_info =
        vk::DescriptorImageInfo {.sampler = *clamp_sampler};
    auto repeat_sampler_info =
        vk::DescriptorImageInfo {.sampler = *repeat_sampler};

    auto geometry_buffer_info = vk::DescriptorBufferInfo {
        .buffer = geometry_buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    auto uniform_buffer_info = vk::DescriptorBufferInfo {
        .buffer = uniform_buffer.buffer.buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE};

    // Write initial descriptor sets.
    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet {
                .dstSet = *descriptor_set.set,
                .dstBinding = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &geometry_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *descriptor_set.set,
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &uniform_buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = *descriptor_set.set,
                .dstBinding = 2,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &image_info},
            vk::WriteDescriptorSet {
                .dstSet = *descriptor_set.set,
                .dstBinding = 3,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &clamp_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *descriptor_set.set,
                .dstBinding = 4,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &lut_image_info},
            vk::WriteDescriptorSet {
                .dstSet = *descriptor_set.set,
                .dstBinding = 5,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &repeat_sampler_info},
        },
        {}
    );

    auto tracy_ctx =
        TracyVkContext(*phys_device, *device, *graphics_queue, *command_buffer);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int current_width, current_height;
        glfwGetWindowSize(window, &current_width, &current_height);
        vk::Extent2D current_extent = {
            .width = static_cast<uint32_t>(current_width),
            .height = static_cast<uint32_t>(current_height)};
        if (extent != current_extent) {
            graphics_queue.waitIdle();

            extent = current_extent;

            swapchain_create_info.imageExtent = extent;
            swapchain_create_info.oldSwapchain = *swapchain;

            // todo: this prints a validation error on resize. Still works fine though.
            // ideally https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_swapchain_maintenance1.html is used to avoid this.
            swapchain = device.createSwapchainKHR(swapchain_create_info);

            swapchain_images = swapchain.getImages();
            swapchain_image_views = create_swapchain_image_views(
                device,
                swapchain_images,
                swapchain_create_info.imageFormat
            );

            scene_referred_framebuffer = ImageWithView(
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
            );

            image_info = vk::DescriptorImageInfo {
                .imageView = *scene_referred_framebuffer.view,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

            device.updateDescriptorSets(
                {vk::WriteDescriptorSet {
                    .dstSet = *descriptor_set.set,
                    .dstBinding = 2,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eSampledImage,
                    .pImageInfo = &image_info}},
                {}
            );

            depthbuffer = ImageWithView(
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
                 .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment},
                allocator,
                device,
                "depthbuffer",
                DEPTH_SUBRESOURCE_RANGE
            );
        }

        time += 1.0 / 60.0;

        // Wait on the render fence to be signaled
        // (it's signaled before this loop starts so that we don't just block forever on the first frame)
        check_vk_result(device.waitForFences({*render_fence}, true, u64_max));
        device.resetFences({*render_fence});

        // Important! This needs to happen after waiting on the fence because otherwise we get race conditions
        // due to writing to a value that the previous frame is reading from.
        {
            auto view = glm::lookAt(
                glm::vec3(sin(time / 2.0) * 100, 100, cos(time / 2.0) * 100),
                glm::vec3(0, 0, 0),
                glm::vec3(0, 1, 0)
            );

            auto perspective = infinite_reverse_z_perspective(
                glm::radians(45.0f),
                float(extent.width),
                float(extent.height),
                0.01
            );

            auto matrix = perspective * view;

            std::memcpy(uniform_buffer.mapped_ptr, &matrix, sizeof(matrix));
        }

        // Reset the command pool instead of resetting the single command buffer as
        // it's cheaper (afaik). Obviously don't do this if multiple command buffers are used.
        command_pool.reset();

        // Acquire the next swapchain image (waiting on the gpu-side and signaling the present semaphore when finished).
        auto [acquire_err, swapchain_image_index] =
            swapchain.acquireNextImage(u64_max, *present_semaphore);

        // This wraps vkBeginCommandBuffer.
        command_buffer.begin(
            {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit}
        );

        {
            TracyVkZone(tracy_ctx, *command_buffer, "main")

                command_buffer.setScissor(
                    0,
                    {vk::Rect2D {.offset = {}, .extent = extent}}
                );
            command_buffer.setViewport(
                0,
                {vk::Viewport {
                    .width = static_cast<float>(extent.width),
                    .height = static_cast<float>(extent.height),
                    .minDepth = 0.0,
                    .maxDepth = 1.0}}
            );
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                *pipelines.pipeline_layout,
                0,
                {*descriptor_set.set},
                {}
            );

            insert_color_image_barriers(
                command_buffer,
                std::array {
                    ImageBarrier {
                        .prev_access = THSVS_ACCESS_NONE,
                        .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                        .discard_contents = true,
                        .queue_family = graphics_queue_family,
                        .image = scene_referred_framebuffer.image.image},
                    ImageBarrier {
                        .prev_access = THSVS_ACCESS_NONE,
                        .next_access =
                            THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                        .discard_contents = true,
                        .queue_family = graphics_queue_family,
                        .image = depthbuffer.image.image,
                        .subresource_range = DEPTH_SUBRESOURCE_RANGE}}
            );

            vk::RenderingAttachmentInfoKHR framebuffer_attachment_info = {
                .imageView = *scene_referred_framebuffer.view,
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
            };
            vk::RenderingAttachmentInfoKHR depth_attachment_info = {
                .imageView = *depthbuffer.view,
                .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
            };
            command_buffer.beginRendering(
                {.renderArea =
                     {
                         .offset = {},
                         .extent = extent,
                     },
                 .layerCount = 1,
                 .colorAttachmentCount = 1,
                 .pColorAttachments = &framebuffer_attachment_info,
                 .pDepthAttachment = &depth_attachment_info}
            );
            // Depth pre-pass
            {
                TracyVkZone(tracy_ctx, *command_buffer, "depth pre pass")

                    command_buffer.bindPipeline(
                        vk::PipelineBindPoint::eGraphics,
                        *pipelines.geometry_depth_prepass
                    );
                command_buffer.draw(powerplant.num_indices, 1, 0, 0);
            }
            {
                TracyVkZone(tracy_ctx, *command_buffer, "main pass")

                    command_buffer.bindPipeline(
                        vk::PipelineBindPoint::eGraphics,
                        *pipelines.render_geometry
                    );
                command_buffer.draw(powerplant.num_indices, 1, 0, 0);
            }
            command_buffer.endRendering();

            insert_color_image_barriers(
                command_buffer,
                std::array {
                    ImageBarrier {
                        .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                        .next_access =
                            THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                        .discard_contents = false,
                        .queue_family = graphics_queue_family,
                        .image = scene_referred_framebuffer.image.image},
                    ImageBarrier {
                        .prev_access = THSVS_ACCESS_NONE,
                        .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                        .discard_contents = true,
                        .queue_family = graphics_queue_family,
                        .image = swapchain_images[swapchain_image_index]}}
            );

            vk::RenderingAttachmentInfoKHR color_attachment_info = {
                .imageView = *swapchain_image_views[swapchain_image_index],
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eDontCare,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = {}};
            // This just clears the swapchain image with the clear color.
            // Wraps vkCmdBeginRendering
            command_buffer.beginRendering(
                {.renderArea =
                     {
                         .offset = {},
                         .extent = extent,
                     },
                 .layerCount = 1,
                 .colorAttachmentCount = 1,
                 .pColorAttachments = &color_attachment_info}
            );

            command_buffer.bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                *pipelines.display_transform
            );

            command_buffer.draw(3, 1, 0, 0);

            command_buffer.endRendering();

            // Transition the swapchain image from being used as a color attachment
            // to presenting. Don't discard contents!!
            insert_color_image_barriers(
                command_buffer,
                std::array {ImageBarrier {
                    .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                    .next_access = THSVS_ACCESS_PRESENT,
                    .discard_contents = false,
                    .queue_family = graphics_queue_family,
                    .image = swapchain_images[swapchain_image_index]}}
            );
        }
        TracyVkCollect(tracy_ctx, *command_buffer);

        command_buffer.end();

        vk::PipelineStageFlags dst_stage_mask =
            vk::PipelineStageFlagBits::eTransfer;
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
        check_vk_result(graphics_queue.presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*render_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &*swapchain,
            .pImageIndices = &swapchain_image_index,
        }));

        FrameMark;
    }

    // Wait until the device is idle so that we don't get destructor warnings about currently in-use resources.
    device.waitIdle();

    // Todo: this needs to happen after all the allocated memory is destructed.
    // The allocator should be placed in a raii wrapper before any allocations are made.
    allocator.destroy();

    return 0;
}
