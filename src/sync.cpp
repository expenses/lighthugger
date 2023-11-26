#include "sync.h"


void insert_global_barrier(
    const vk::raii::CommandBuffer& command_buffer,
    ThsvsAccessType prev_access,
    ThsvsAccessType next_access
) {
    ThsvsGlobalBarrier global_barrier = {
        .prevAccessCount = 1,
        .pPrevAccesses = &prev_access,
        .nextAccessCount = 1,
        .pNextAccesses = &next_access};

    thsvsCmdPipelineBarrier(
        *command_buffer,
        &global_barrier,
        0,
        nullptr,
        0,
        nullptr
    );
}
