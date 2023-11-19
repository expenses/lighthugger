#include "staging.h"

AllocatedBuffer upload_via_staging_buffer(
    const void* bytes,
    size_t num_bytes,
    vma::Allocator allocator,
    vk::BufferUsageFlags desired_flags,
    const std::string& name,
    const vk::raii::CommandBuffer& command_buffer,
    std::vector<AllocatedBuffer>& temp_buffers
) {
    auto staging_buffer_name = name + " staging buffer";
    auto staging_buffer = PersistentlyMappedBuffer(AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = num_bytes,
            .usage = vk::BufferUsageFlagBits::eTransferSrc},
        {
            .flags = vma::AllocationCreateFlagBits::eMapped
                | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator,
        staging_buffer_name.data()
    ));
    std::memcpy(staging_buffer.mapped_ptr, bytes, num_bytes);

    auto final_buffer = AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = num_bytes,
            .usage = vk::BufferUsageFlagBits::eTransferDst | desired_flags},
        {
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator,
        name.data()
    );

    command_buffer.copyBuffer(
        staging_buffer.buffer.buffer,
        final_buffer.buffer,
        {vk::BufferCopy {.srcOffset = 0, .dstOffset = 0, .size = num_bytes}}
    );

    temp_buffers.push_back(std::move(staging_buffer.buffer));

    return final_buffer;
}
