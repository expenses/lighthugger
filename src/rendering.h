
uint32_t dispatch_size(uint32_t width, uint32_t workgroup_size) {
    return static_cast<uint32_t>(std::ceil(float(width) / float(workgroup_size))
    );
}

void set_scissor_and_viewport(
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t width,
    uint32_t height
) {
    command_buffer.setScissor(
        0,
        {vk::Rect2D {
            .offset = {},
            .extent = vk::Extent2D {.width = width, .height = height}}}
    );
    command_buffer.setViewport(
        0,
        {vk::Viewport {
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.0,
            .maxDepth = 1.0}}
    );
}

void render(
    const vk::raii::CommandBuffer& command_buffer,
    const Pipelines& pipelines,
    const DescriptorSet& descriptor_set,
    const Resources& resources,
    vk::Image swapchain_image,
    const vk::raii::ImageView& swapchain_image_view,
    vk::Extent2D extent,
    uint32_t graphics_queue_family,
    tracy::VkCtx* tracy_ctx
) {
    TracyVkZone(tracy_ctx, *command_buffer, "main");

    {
        command_buffer
            .fillBuffer(resources.depth_info_buffer.buffer, 0, 4, u32_max);
        command_buffer.fillBuffer(resources.depth_info_buffer.buffer, 4, 4, 0);
    }

    set_scissor_and_viewport(command_buffer, extent.width, extent.height);
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelines.pipeline_layout,
        0,
        {*descriptor_set.set},
        {}
    );
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
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
                .image =
                    resources.resizing.scene_referred_framebuffer.image.image},
            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = resources.resizing.depthbuffer.image.image,
                .subresource_range = DEPTH_SUBRESOURCE_RANGE},

            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = resources.shadowmap.image.image,
                .subresource_range = DEPTH_SUBRESOURCE_RANGE}

        }
    );

    // Depth pre-pass
    {
        TracyVkZone(tracy_ctx, *command_buffer, "depth pre pass");

        vk::RenderingAttachmentInfoKHR depth_attachment_info = {
            .imageView = *resources.resizing.depthbuffer.view,
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
             .pDepthAttachment = &depth_attachment_info}
        );

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *pipelines.geometry_depth_prepass
        );
        command_buffer.drawIndirect(
            resources.draw_calls_buffer.buffer,
            0,
            resources.num_draws,
            sizeof(vk::DrawIndirectCommand)
        );
        command_buffer.endRendering();
    }

    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
            .next_access =
                THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
            .discard_contents = false,
            .queue_family = graphics_queue_family,
            .image = resources.resizing.depthbuffer.image.image,
            .subresource_range = DEPTH_SUBRESOURCE_RANGE}}
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "depth reduction");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.read_depth
        );
        command_buffer.dispatch(
            dispatch_size(extent.width, 8),
            dispatch_size(extent.height, 8),
            1
        );
    }
    {
        TracyVkZone(tracy_ctx, *command_buffer, "generate_matrices");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.generate_matrices
        );
        command_buffer.dispatch(1, 1, 1);
    }

    {
        TracyVkZone(tracy_ctx, *command_buffer, "shadowmap rendering");

        set_scissor_and_viewport(command_buffer, 1024, 1024);

        vk::RenderingAttachmentInfoKHR depth_attachment_info = {
            .imageView = *resources.shadowmap.view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {.depthStencil = {.depth = 1.0f}}};
        command_buffer.beginRendering(
            {.renderArea =
                 {
                     .offset = {},
                     .extent = vk::Extent2D {.width = 1024, .height = 1024},
                 },
             .layerCount = 1,
             .pDepthAttachment = &depth_attachment_info}
        );
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *pipelines.shadow_pass
        );
        command_buffer.drawIndirect(
            resources.draw_calls_buffer.buffer,
            0,
            resources.num_draws,
            sizeof(vk::DrawIndirectCommand)
        );
        command_buffer.endRendering();
    }
    set_scissor_and_viewport(command_buffer, extent.width, extent.height);
    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
            .next_access =
                THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
            .discard_contents = false,
            .queue_family = graphics_queue_family,
            .image = resources.shadowmap.image.image,
            .subresource_range = DEPTH_SUBRESOURCE_RANGE}

        }
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "main pass");

        vk::RenderingAttachmentInfoKHR framebuffer_attachment_info = {
            .imageView = *resources.resizing.scene_referred_framebuffer.view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
        };
        auto depth_attachment_info = vk::RenderingAttachmentInfoKHR {
            .imageView = *resources.resizing.depthbuffer.view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
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

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *pipelines.render_geometry
        );
        command_buffer.drawIndirect(
            resources.draw_calls_buffer.buffer,
            0,
            resources.num_draws,
            sizeof(vk::DrawIndirectCommand)
        );
        command_buffer.endRendering();
    }

    insert_color_image_barriers(
        command_buffer,
        std::array {
            ImageBarrier {
                .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                .next_access =
                    THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                .discard_contents = false,
                .queue_family = graphics_queue_family,
                .image =
                    resources.resizing.scene_referred_framebuffer.image.image},
            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = swapchain_image},
        }
    );

    {
        vk::RenderingAttachmentInfoKHR color_attachment_info = {
            .imageView = *swapchain_image_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {}};
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

        {
            TracyVkZone(tracy_ctx, *command_buffer, "display_transform");
            command_buffer.draw(3, 1, 0, 0);
        }

        {
            TracyVkZone(tracy_ctx, *command_buffer, "imgui");
            ImDrawData* draw_data = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(draw_data, *command_buffer);
        }

        command_buffer.endRendering();
    }

    // Transition the swapchain image from being used as a color attachment
    // to presenting. Don't discard contents!!
    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
            .next_access = THSVS_ACCESS_PRESENT,
            .discard_contents = false,
            .queue_family = graphics_queue_family,
            .image = swapchain_image}}
    );
}
