#include "../allocations/base.h"
#include "../descriptor_set.h"
#include "../shared_cpu_gpu.h"

struct BoundingBox {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());

    void insert(glm::vec3 point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    float diagonal_length() {
        return glm::distance(min, max);
    }
};

struct Mesh {
    AllocatedBuffer positions;
    AllocatedBuffer indices;
    AllocatedBuffer normals;
    AllocatedBuffer material_ids;
    AllocatedBuffer uvs;
    AllocatedBuffer material_info;
    std::vector<ImageWithView> images;
    std::vector<uint32_t> image_indices;
    uint32_t num_indices;
    float bounding_sphere_radius;
    BoundingBox bounding_box;

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
