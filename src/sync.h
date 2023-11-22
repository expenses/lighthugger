#include "util.h"

// Make inserting color image transition barriers easier.

struct ImageBarrier {
    ThsvsAccessType prev_access;
    ThsvsAccessType next_access;
    ThsvsImageLayout prev_layout = THSVS_IMAGE_LAYOUT_OPTIMAL;
    ThsvsImageLayout next_layout = THSVS_IMAGE_LAYOUT_OPTIMAL;
    bool discard_contents = false;
    uint32_t queue_family;
    vk::Image image;
    vk::ImageSubresourceRange subresource_range = COLOR_SUBRESOURCE_RANGE;
};

template<size_t N>
void insert_color_image_barriers(
    const vk::raii::CommandBuffer& command_buffer,
    const std::array<ImageBarrier, N>& barriers
) {
    std::array<ThsvsImageBarrier, N> thsvs_barriers;

    for (size_t i = 0; i < thsvs_barriers.size(); i++) {
        thsvs_barriers[i] = {
            .prevAccessCount = 1,
            .pPrevAccesses = &barriers[i].prev_access,
            .nextAccessCount = 1,
            .pNextAccesses = &barriers[i].next_access,
            .prevLayout = barriers[i].prev_layout,
            .nextLayout = barriers[i].next_layout,
            .discardContents = barriers[i].discard_contents,
            .srcQueueFamilyIndex = barriers[i].queue_family,
            .dstQueueFamilyIndex = barriers[i].queue_family,
            .image = barriers[i].image,
            .subresourceRange = barriers[i].subresource_range};
    }

    thsvsCmdPipelineBarrier(
        *command_buffer,
        nullptr,
        0,
        nullptr,
        thsvs_barriers.size(),
        thsvs_barriers.data()
    );
}
