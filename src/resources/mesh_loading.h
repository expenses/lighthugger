#pragma once
#include "../allocations/base.h"
#include "../descriptor_set.h"
#include "../pipelines.h"
#include "../shared_cpu_gpu.h"
#include "meshlets.h"

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

struct GltfPrimitive {
    AllocatedBuffer position;
    AllocatedBuffer indices;
    AllocatedBuffer uvs;
    AllocatedBuffer normals;
    AllocatedBuffer mesh_info;
    AllocatedBuffer micro_indices;
    AllocatedBuffer meshlets;
    glm::mat4 transform;
    uint32_t num_meshlets;
};

struct GltfMesh {
    std::vector<ImageWithView> images;
    std::vector<uint32_t> image_indices;
    std::vector<GltfPrimitive> primitives;
    std::shared_ptr<IndexTracker> image_index_tracker;

    ~GltfMesh();
};

GltfMesh load_gltf(
    const std::filesystem::path& filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers,
    DescriptorSet& descriptor_set,
    const Pipelines& pipelines
);
