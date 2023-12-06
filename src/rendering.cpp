#include "rendering.h"

#include "sync.h"

const auto u32_max = std::numeric_limits<uint32_t>::max();

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
    tracy::VkCtx* tracy_ctx,
    uint32_t swapchain_image_index
) {
    ZoneScoped;
    TracyVkZone(tracy_ctx, *command_buffer, "render");

    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipelines.pipeline_layout,
        0,
        {*descriptor_set.set,
         *descriptor_set.swapchain_image_sets[swapchain_image_index]},
        {}
    );
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelines.pipeline_layout,
        0,
        {*descriptor_set.set},
        {}
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "buffer clears");

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.reset_buffers
        );
        command_buffer.dispatch(1, 1, 1);
    }

    insert_color_image_barriers(
        command_buffer,
        std::array {
            // Get depth buffer ready for rendering.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = resources.resizing.depthbuffer.image.image,
                .subresource_range = DEPTH_SUBRESOURCE_RANGE},
            // Get shadowmaps ready for rendering.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = resources.shadowmap.image.image,
                .subresource_range =
                    {
                        .aspectMask = vk::ImageAspectFlagBits::eDepth,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 4,
                    }},
            // Get framebuffer ready for writing
            ImageBarrier {
                .prev_access = THSVS_ACCESS_COMPUTE_SHADER_WRITE,
                .next_access = THSVS_ACCESS_COMPUTE_SHADER_WRITE,
                .next_layout = THSVS_IMAGE_LAYOUT_GENERAL,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image =
                    resources.resizing.scene_referred_framebuffer.image.image},
            // Get swapchain image ready for rendering.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_COMPUTE_SHADER_WRITE,
                .next_access = THSVS_ACCESS_COMPUTE_SHADER_WRITE,
                .next_layout = THSVS_IMAGE_LAYOUT_GENERAL,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = swapchain_image},
            // Get visbuffer image ready for rendering.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_NONE,
                .next_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                .discard_contents = true,
                .queue_family = graphics_queue_family,
                .image = resources.resizing.visbuffer.image.image},
        },
        std::optional(GlobalBarrier<1, 1> {
            .prev_accesses = {THSVS_ACCESS_TRANSFER_WRITE},
            .next_accesses = {THSVS_ACCESS_COMPUTE_SHADER_READ_OTHER}})
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "gather meshlets from instances");

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.expand_meshlets
        );
        command_buffer
            .dispatch(dispatch_size(resources.total_num_meshlets, 64), 1, 1);
    }

    insert_global_barrier(
        command_buffer,
        GlobalBarrier<1, 1> {
            .prev_accesses =
                std::array<ThsvsAccessType, 1> {
                    THSVS_ACCESS_COMPUTE_SHADER_WRITE},
            .next_accesses =
                std::array<ThsvsAccessType, 1> {
                    THSVS_ACCESS_COMPUTE_SHADER_READ_OTHER}}
    );

    {
        TracyVkZone(
            tracy_ctx,
            *command_buffer,
            "cull meshlets and write draw calls"
        );

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.write_draw_calls
        );
        command_buffer.dispatch(
            dispatch_size(MAX_OPAQUE_DRAWS + MAX_ALPHA_CLIP_DRAWS, 64),
            1,
            1
        );
    }

    insert_global_barrier(
        command_buffer,
        GlobalBarrier<1, 1> {
            .prev_accesses =
                std::array<ThsvsAccessType, 1> {
                    THSVS_ACCESS_COMPUTE_SHADER_WRITE},
            .next_accesses =
                std::array<ThsvsAccessType, 1> {THSVS_ACCESS_INDIRECT_BUFFER}}
    );

    set_scissor_and_viewport(command_buffer, extent.width, extent.height);

    {
        TracyVkZone(tracy_ctx, *command_buffer, "visbuffer rendering");

        vk::RenderingAttachmentInfoKHR visbuffer_attachment_info = {
            .imageView = *resources.resizing.visbuffer.view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eDontCare,
            .storeOp = vk::AttachmentStoreOp::eStore,
        };
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
             .colorAttachmentCount = 1,
             .pColorAttachments = &visbuffer_attachment_info,
             .pDepthAttachment = &depth_attachment_info}
        );

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *pipelines.rasterize_visbuffer.opaque
        );
        {
            TracyVkZone(
                tracy_ctx,
                *command_buffer,
                "visbuffer: opaque geometry"
            );

            command_buffer.drawIndirectCount(
                resources.draw_calls_buffer.buffer,
                DRAW_CALLS_COUNTS_SIZE,
                resources.draw_calls_buffer.buffer,
                0,
                MAX_OPAQUE_DRAWS,
                sizeof(vk::DrawIndirectCommand)
            );
        }
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eGraphics,
            *pipelines.rasterize_visbuffer.alpha_clip
        );
        {
            TracyVkZone(
                tracy_ctx,
                *command_buffer,
                "visbuffer: alpha clip geometry"
            );

            command_buffer.drawIndirectCount(
                resources.draw_calls_buffer.buffer,
                DRAW_CALLS_COUNTS_SIZE
                    + ALPHA_CLIP_DRAWS_OFFSET * sizeof(vk::DrawIndirectCommand),
                resources.draw_calls_buffer.buffer,
                sizeof(uint32_t) * 4,
                MAX_ALPHA_CLIP_DRAWS,
                sizeof(vk::DrawIndirectCommand)
            );
        }
        command_buffer.endRendering();
    }

    insert_color_image_barriers(
        command_buffer,
        std::array {
            // Switch depthbuffer from write to read.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                .next_access =
                    THSVS_ACCESS_COMPUTE_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                .queue_family = graphics_queue_family,
                .image = resources.resizing.depthbuffer.image.image,
                .subresource_range = DEPTH_SUBRESOURCE_RANGE},
            // Switch visbuffer from write to read.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
                .next_access =
                    THSVS_ACCESS_COMPUTE_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                .queue_family = graphics_queue_family,
                .image = resources.resizing.visbuffer.image.image},
        }
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "depth reduction");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.read_depth
        );
        command_buffer.dispatch(
            dispatch_size(extent.width, 8 * 4),
            dispatch_size(extent.height, 8 * 4),
            1
        );
    }
    {
        TracyVkZone(tracy_ctx, *command_buffer, "generate shadow matrices and reset draw calls");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.generate_matrices
        );
        command_buffer.dispatch(1, 1, 1);
    }

    {
        TracyVkZone(
            tracy_ctx,
            *command_buffer,
            "cull meshlets and write draw calls"
        );

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.write_draw_calls_shadows
        );
        command_buffer.dispatch(
            dispatch_size(MAX_OPAQUE_DRAWS + MAX_ALPHA_CLIP_DRAWS, 16),
            1,
            1
        );
    }

    insert_global_barrier(
        command_buffer,
        GlobalBarrier<1, 1> {
            .prev_accesses =
                std::array<ThsvsAccessType, 1> {
                    THSVS_ACCESS_COMPUTE_SHADER_WRITE},
            .next_accesses =
                std::array<ThsvsAccessType, 1> {THSVS_ACCESS_INDIRECT_BUFFER}}
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "shadowmap rasterization");

        set_scissor_and_viewport(command_buffer, 1024, 1024);

        for (uint32_t i = 0; i < resources.shadowmap_layer_views.size(); i++) {
            TracyVkZone(tracy_ctx, *command_buffer, "shadowmap inner");

            vk::RenderingAttachmentInfoKHR depth_attachment_info = {
                .imageView = *resources.shadowmap_layer_views[i],
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
            command_buffer.pushConstants<ShadowPassConstant>(
                *pipelines.pipeline_layout,
                vk::ShaderStageFlagBits::eVertex
                    | vk::ShaderStageFlagBits::eCompute,
                0,
                {{.cascade_index = i}}
            );
            command_buffer.bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                *pipelines.rasterize_shadowmap.opaque
            );
            {
                TracyVkZone(
                    tracy_ctx,
                    *command_buffer,
                    "shadowmap: opaque geometry"
                );

                command_buffer.drawIndirectCount(
                    resources.draw_calls_buffer.buffer,
                    DRAW_CALLS_COUNTS_SIZE
                        + (MESHLET_INDICES_BUFFER_SECTION_OFFSET * i)
                            * sizeof(vk::DrawIndirectCommand),
                    resources.draw_calls_buffer.buffer,
                    i * sizeof(uint32_t),
                    MAX_OPAQUE_DRAWS,
                    sizeof(vk::DrawIndirectCommand)
                );
            }
            command_buffer.bindPipeline(
                vk::PipelineBindPoint::eGraphics,
                *pipelines.rasterize_shadowmap.alpha_clip
            );
            {
                TracyVkZone(
                    tracy_ctx,
                    *command_buffer,
                    "shadowmap: alpha clip geometry"
                );

                command_buffer.drawIndirectCount(
                    resources.draw_calls_buffer.buffer,
                    DRAW_CALLS_COUNTS_SIZE
                        + (ALPHA_CLIP_DRAWS_OFFSET
                           + MESHLET_INDICES_BUFFER_SECTION_OFFSET * i)
                            * sizeof(vk::DrawIndirectCommand),
                    resources.draw_calls_buffer.buffer,
                    sizeof(uint32_t) * 4 + i * sizeof(uint32_t),
                    MAX_ALPHA_CLIP_DRAWS,
                    sizeof(vk::DrawIndirectCommand)
                );
            }
            command_buffer.endRendering();
        }
    }

    insert_color_image_barriers(
        command_buffer,
        std::array {
            // Switch shadowmap from write to read.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE,
                .next_access =
                    THSVS_ACCESS_COMPUTE_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                .queue_family = graphics_queue_family,
                .image = resources.shadowmap.image.image,
                .subresource_range =
                    {
                        .aspectMask = vk::ImageAspectFlagBits::eDepth,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 4,
                    }}

        }
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "render geometry");

        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.render_geometry
        );
        command_buffer.dispatch(
            dispatch_size(extent.width, 8),
            dispatch_size(extent.height, 8),
            1
        );
    }

    insert_color_image_barriers(
        command_buffer,
        std::array {
            // Switch framebuffer from write to read.
            ImageBarrier {
                .prev_access = THSVS_ACCESS_COMPUTE_SHADER_WRITE,
                .next_access =
                    THSVS_ACCESS_COMPUTE_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
                .prev_layout = THSVS_IMAGE_LAYOUT_GENERAL,
                .queue_family = graphics_queue_family,
                .image =
                    resources.resizing.scene_referred_framebuffer.image.image},
        }
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "display transform");
        command_buffer.bindPipeline(
            vk::PipelineBindPoint::eCompute,
            *pipelines.display_transform
        );
        command_buffer.dispatch(
            dispatch_size(extent.width, 8),
            dispatch_size(extent.height, 8),
            1
        );
    }

    insert_global_barrier(
        command_buffer,
        GlobalBarrier<1, 1> {
            .prev_accesses = {THSVS_ACCESS_COMPUTE_SHADER_WRITE},
            .next_accesses = {THSVS_ACCESS_COLOR_ATTACHMENT_WRITE}}
    );

    {
        TracyVkZone(tracy_ctx, *command_buffer, "imgui");

        vk::RenderingAttachmentInfoKHR color_attachment_info = {
            .imageView = *swapchain_image_view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eLoad,
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

        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, *command_buffer);

        command_buffer.endRendering();
    }

    // Transition the swapchain image from being used as a color attachment
    // to presenting. Don't discard contents!!
    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_COLOR_ATTACHMENT_WRITE,
            .next_access = THSVS_ACCESS_PRESENT,
            .prev_layout = THSVS_IMAGE_LAYOUT_GENERAL,
            .queue_family = graphics_queue_family,
            .image = swapchain_image}}
    );
}
