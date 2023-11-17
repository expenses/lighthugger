#include "allocations.h"
#include "shared_cpu_gpu.h"

struct Mesh {
    AllocatedBuffer vertices;
    AllocatedBuffer indices;
    AllocatedBuffer normals;
    AllocatedBuffer material_ids;
    uint32_t num_indices;

    MeshBufferAddresses get_addresses(const vk::raii::Device& device);
};

Mesh load_obj(const char* filepath, vma::Allocator allocator);
