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
