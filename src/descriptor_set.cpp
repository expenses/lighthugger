#include "descriptor_set.h"

IndexTracker::IndexTracker() {}

uint32_t IndexTracker::push() {
    if (!free_indices.empty()) {
        uint32_t index = free_indices.back();
        free_indices.pop_back();
        return index;
    }

    uint32_t index = next_index;
    next_index += 1;

    return index;
}

void IndexTracker::free(uint32_t index) {
    free_indices.push_back(index);
}

IndexTracker::~IndexTracker() {
    // Ensure that we've freed all images.
    assert(free_indices.size() == next_index);
}

DescriptorSetLayouts
create_descriptor_set_layouts(const vk::raii::Device& device) {
    auto everything_bindings = std::array {
        // Bindless images
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 512,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eFragment,
        },
        // scene_referred_framebuffer
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // display transform LUT
        vk::DescriptorSetLayoutBinding {
            .binding = 2,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // depthbuffer
        vk::DescriptorSetLayoutBinding {
            .binding = 3,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // shadow map
        vk::DescriptorSetLayoutBinding {
            .binding = 4,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // rw scene referred framebuffer
        vk::DescriptorSetLayoutBinding {
            .binding = 5,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // visibility buffer
        vk::DescriptorSetLayoutBinding {
            .binding = 6,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // clamp sampler
        vk::DescriptorSetLayoutBinding {
            .binding = 7,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // repeat sampler
        vk::DescriptorSetLayoutBinding {
            .binding = 8,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute
                | vk::ShaderStageFlagBits::eFragment,
        },
        // Shadowmap comparison sampler
        vk::DescriptorSetLayoutBinding {
            .binding = 9,
            .descriptorType = vk::DescriptorType::eSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
        // skybox
        vk::DescriptorSetLayoutBinding {
            .binding = 10,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        },
    };

    std::vector<vk::DescriptorBindingFlags> flags(everything_bindings.size());
    // Set the images as being partially bound, so not all slots have to be used.
    flags[0] = vk::DescriptorBindingFlagBits::ePartiallyBound;

    auto flags_create_info = vk::DescriptorSetLayoutBindingFlagsCreateInfo {
        .bindingCount = static_cast<uint32_t>(flags.size()),
        .pBindingFlags = flags.data()};

    auto swapchain_storage_image_bindings = std::array {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute},
    };

    return DescriptorSetLayouts {
        .everything = device.createDescriptorSetLayout({
            .pNext = &flags_create_info,
            .bindingCount = everything_bindings.size(),
            .pBindings = everything_bindings.data(),
        }),
        .swapchain_storage_image = device.createDescriptorSetLayout({
            .bindingCount = swapchain_storage_image_bindings.size(),
            .pBindings = swapchain_storage_image_bindings.data(),
        }),
    };
}

uint32_t
DescriptorSet::write_image(const ImageWithView& image, vk::Device device) {
    auto index = tracker->push();

    if (index >= 512) {
        dbg(index);
        abort();
    }

    auto image_info = vk::DescriptorImageInfo {
        .imageView = *image.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    device.updateDescriptorSets(
        {vk::WriteDescriptorSet {
            .dstSet = *set,
            .dstBinding = 0,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_info}},
        {}
    );

    return index;
}

DescriptorSet::DescriptorSet(
    vk::raii::DescriptorSet set_,
    std::vector<vk::raii::DescriptorSet> swapchain_image_sets_
) :
    set(std::move(set_)),
    swapchain_image_sets(std::move(swapchain_image_sets_)) {}

void DescriptorSet::write_resizing_descriptors(
    const ResizingResources& resizing_resources,
    const vk::raii::Device& device,
    const std::vector<vk::raii::ImageView>& swapchain_image_views
) {
    auto image_info = vk::DescriptorImageInfo {
        .imageView = *resizing_resources.scene_referred_framebuffer.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto depthbuffer_image_info = vk::DescriptorImageInfo {
        .imageView = *resizing_resources.depthbuffer.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto rw_scene_referred_framebuffer_info = vk::DescriptorImageInfo {
        .imageView = *resizing_resources.scene_referred_framebuffer.view,
        .imageLayout = vk::ImageLayout::eGeneral};

    auto visbuffer_image_info = vk::DescriptorImageInfo {
        .imageView = *resizing_resources.visbuffer.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    device.updateDescriptorSets(
        {vk::WriteDescriptorSet {
             .dstSet = *set,
             .dstBinding = 1,
             .descriptorCount = 1,
             .descriptorType = vk::DescriptorType::eSampledImage,
             .pImageInfo = &image_info},
         vk::WriteDescriptorSet {
             .dstSet = *set,
             .dstBinding = 3,
             .descriptorCount = 1,
             .descriptorType = vk::DescriptorType::eSampledImage,
             .pImageInfo = &depthbuffer_image_info},
         vk::WriteDescriptorSet {
             .dstSet = *set,
             .dstBinding = 5,
             .descriptorCount = 1,
             .descriptorType = vk::DescriptorType::eStorageImage,
             .pImageInfo = &rw_scene_referred_framebuffer_info},
         vk::WriteDescriptorSet {
             .dstSet = *set,
             .dstBinding = 6,
             .descriptorCount = 1,
             .descriptorType = vk::DescriptorType::eSampledImage,
             .pImageInfo = &visbuffer_image_info}},
        {}
    );

    for (uint32_t i = 0; i < swapchain_image_views.size(); i++) {
        auto swapchain_image_info = vk::DescriptorImageInfo {
            .imageView = *swapchain_image_views[i],
            .imageLayout = vk::ImageLayout::eGeneral};

        device.updateDescriptorSets(
            {
                vk::WriteDescriptorSet {
                    .dstSet = *swapchain_image_sets[i],
                    .dstBinding = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eStorageImage,
                    .pImageInfo = &swapchain_image_info},
            },
            {}
        );
    }
}

void DescriptorSet::write_descriptors(
    const Resources& resources,
    const vk::raii::Device& device,
    const std::vector<vk::raii::ImageView>& swapchain_image_views
) {
    write_resizing_descriptors(
        resources.resizing,
        device,
        swapchain_image_views
    );

    auto lut_image_info = vk::DescriptorImageInfo {
        .imageView = *resources.display_transform_lut.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto shadowmap_image_info = vk::DescriptorImageInfo {
        .imageView = *resources.shadowmap.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    auto clamp_sampler_info =
        vk::DescriptorImageInfo {.sampler = *resources.clamp_sampler};
    auto repeat_sampler_info =
        vk::DescriptorImageInfo {.sampler = *resources.repeat_sampler};
    auto shadowmap_comparison_sampler_info = vk::DescriptorImageInfo {
        .sampler = *resources.shadowmap_comparison_sampler};

    auto skybox_image_info = vk::DescriptorImageInfo {
        .imageView = *resources.skybox.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    // Write initial descriptor sets.
    device.updateDescriptorSets(
        {
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 2,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &lut_image_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 4,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &shadowmap_image_info},

            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 7,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &clamp_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 8,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &repeat_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 9,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampler,
                .pImageInfo = &shadowmap_comparison_sampler_info},
            vk::WriteDescriptorSet {
                .dstSet = *set,
                .dstBinding = 10,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eSampledImage,
                .pImageInfo = &skybox_image_info},

        },
        {}
    );
}
