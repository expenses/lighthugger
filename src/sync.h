#include "util.h"

// Make inserting color image transition barriers easier.

struct ImageBarrier {
    ThsvsAccessType prev_access;
    ThsvsAccessType next_access;
    //ThsvsImageLayout prev_layout;
    //ThsvsImageLayout next_layout;
    bool discard_contents;
    uint32_t queue_family;
    vk::Image image;
};

void insert_color_image_barrier(const vk::raii::CommandBuffer& command_buffer, ImageBarrier barrier) {
    ThsvsImageBarrier image_barrier = {
        .prevAccessCount = 1,
        .pPrevAccesses = &barrier.prev_access,
        .nextAccessCount = 1,
        .pNextAccesses = &barrier.next_access,
        .prevLayout = THSVS_IMAGE_LAYOUT_OPTIMAL,//barrier.prev_layout,
        .nextLayout = THSVS_IMAGE_LAYOUT_OPTIMAL,//barrier.next_layout,
        .discardContents = barrier.discard_contents,
        .srcQueueFamilyIndex = barrier.queue_family,
        .dstQueueFamilyIndex = barrier.queue_family,
        .image = barrier.image,
        .subresourceRange = COLOR_SUBRESOURCE_RANGE
    };
    thsvsCmdPipelineBarrier(*command_buffer, nullptr, 0, nullptr, 1, &image_barrier);
}

template<size_t N>
void insert_color_image_barriers(const vk::raii::CommandBuffer& command_buffer, const std::array<ImageBarrier, N>& barriers) {
    std::array<ThsvsImageBarrier, N> thsvs_barriers;

    for (size_t i = 0; i < thsvs_barriers.size(); i++) {
        thsvs_barriers[i] = {
            .prevAccessCount = 1,
            .pPrevAccesses = &barriers[i].prev_access,
            .nextAccessCount = 1,
            .pNextAccesses = &barriers[i].next_access,
            .prevLayout = THSVS_IMAGE_LAYOUT_OPTIMAL,//barrier.prev_layout,
            .nextLayout = THSVS_IMAGE_LAYOUT_OPTIMAL,//barrier.next_layout,
            .discardContents = barriers[i].discard_contents,
            .srcQueueFamilyIndex = barriers[i].queue_family,
            .dstQueueFamilyIndex = barriers[i].queue_family,
            .image = barriers[i].image,
            .subresourceRange = COLOR_SUBRESOURCE_RANGE
        };
    }

    thsvsCmdPipelineBarrier(*command_buffer, nullptr, 0, nullptr, thsvs_barriers.size(), thsvs_barriers.data());
}
