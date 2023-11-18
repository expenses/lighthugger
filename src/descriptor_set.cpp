#include "descriptor_set.h"

uint32_t
DescriptorSet::write_image(const ImageWithView& image, vk::Device device) {
    auto index = tracker.push();

    auto image_info = vk::DescriptorImageInfo {
        .imageView = *image.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    device.updateDescriptorSets(
        {vk::WriteDescriptorSet {
            .dstSet = *set,
            .dstBinding = 9,
            .dstArrayElement = index,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eSampledImage,
            .pImageInfo = &image_info}},
        {}
    );

    return index;
}
