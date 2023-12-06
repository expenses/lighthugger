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

template<size_t P, size_t N>
struct GlobalBarrier {
    std::array<ThsvsAccessType, P> prev_accesses;
    std::array<ThsvsAccessType, N> next_accesses;
};

template<size_t N, size_t GP = 0, size_t GN = 0>
void insert_color_image_barriers(
    const vk::raii::CommandBuffer& command_buffer,
    const std::array<ImageBarrier, N>& barriers,
    std::optional<GlobalBarrier<GP, GN>> opt_global_barrier = std::nullopt
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

    std::optional<ThsvsGlobalBarrier> thsvs_global_barrier = std::nullopt;

    if (opt_global_barrier) {
        auto& global_barrier = opt_global_barrier.value();

        thsvs_global_barrier = {
            .prevAccessCount =
                static_cast<uint32_t>(global_barrier.prev_accesses.size()),
            .pPrevAccesses = global_barrier.prev_accesses.data(),
            .nextAccessCount =
                static_cast<uint32_t>(global_barrier.prev_accesses.size()),
            .pNextAccesses = global_barrier.next_accesses.data()};
    }

    thsvsCmdPipelineBarrier(
        *command_buffer,
        thsvs_global_barrier.has_value() ? &thsvs_global_barrier.value()
                                         : nullptr,
        0,
        nullptr,
        thsvs_barriers.size(),
        thsvs_barriers.data()
    );
}

template<size_t P, size_t N>
void insert_global_barrier(
    const vk::raii::CommandBuffer& command_buffer,
    GlobalBarrier<P, N> global_barrier
) {
    ThsvsGlobalBarrier thsvs_global_barrier = {
        .prevAccessCount =
            static_cast<uint32_t>(global_barrier.prev_accesses.size()),
        .pPrevAccesses = global_barrier.prev_accesses.data(),
        .nextAccessCount =
            static_cast<uint32_t>(global_barrier.prev_accesses.size()),
        .pNextAccesses = global_barrier.next_accesses.data()};

    thsvsCmdPipelineBarrier(
        *command_buffer,
        &thsvs_global_barrier,
        0,
        nullptr,
        0,
        nullptr
    );
}
