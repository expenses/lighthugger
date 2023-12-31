#include "persistently_mapped.h"

AllocatedBuffer upload_via_staging_buffer(
    const void* bytes,
    size_t num_bytes,
    vma::Allocator allocator,
    vk::BufferUsageFlags desired_flags,
    const std::string& name,
    const vk::raii::CommandBuffer& command_buffer,
    std::vector<AllocatedBuffer>& temp_buffers
);

std::pair<AllocatedBuffer, size_t> upload_from_file_via_staging_buffer(
    std::ifstream stream,
    vma::Allocator allocator,
    vk::BufferUsageFlags desired_flags,
    const std::string& name,
    const vk::raii::CommandBuffer& command_buffer,
    std::vector<AllocatedBuffer>& temp_buffers
);
