#include "allocations.h"
#include "descriptor_set.h"
#include "shared_cpu_gpu.h"

struct Mesh {
    AllocatedBuffer vertices;
    AllocatedBuffer indices;
    AllocatedBuffer normals;
    AllocatedBuffer material_ids;
    AllocatedBuffer uvs;
    std::vector<ImageWithView> images;
    uint32_t num_indices;

    MeshBufferAddresses get_addresses(const vk::raii::Device& device);
};

Mesh load_obj(
    const char* filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers,
    DescriptorSet& descriptor_set
);
