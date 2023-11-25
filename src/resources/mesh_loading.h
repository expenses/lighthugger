#pragma once
#include "../allocations/base.h"
#include "../descriptor_set.h"
#include "../pipelines.h"
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
    AllocatedBuffer mesh_info;
    std::vector<ImageWithView> images;
    std::vector<uint32_t> image_indices;
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

struct GltfPrimitive {
    AllocatedBuffer position;
    AllocatedBuffer indices;
    AllocatedBuffer uvs;
    AllocatedBuffer normals;
    AllocatedBuffer mesh_info;
    glm::mat4 transform;
};

struct GltfMesh {
    std::vector<ImageWithView> images;
    std::vector<uint32_t> image_indices;
    AllocatedBuffer material_info;
    std::vector<GltfPrimitive> primitives;
};

GltfMesh load_gltf(
    const std::filesystem::path& filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers,
    DescriptorSet& descriptor_set,
    const Pipelines& pipelines,
    std::vector<DescriptorPoolAndSet>& temp_descriptor_sets
);
