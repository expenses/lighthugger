#pragma once

#ifdef __HLSL_VERSION
    #define MATRIX_TYPE float4x4
    #define VEC3_TYPE float3
#else
    #define MATRIX_TYPE glm::mat4
    #define VEC3_TYPE glm::vec3
#endif

struct MeshBufferAddresses {
    uint64_t positions;
    uint64_t indices;
    uint64_t normals;
    uint64_t uvs;
    uint64_t material_indices;
    uint64_t material_info;
    uint32_t num_indices;
    float bounding_sphere_radius;
    float longest_distance;
};

struct DepthInfoBuffer {
    MATRIX_TYPE shadow_rendering_matrices[4];
    float cascade_splits[4];
    uint32_t min_depth;
    uint32_t max_depth;
};

struct Uniforms {
    MATRIX_TYPE combined_perspective_view;
    MATRIX_TYPE inv_perspective_view;
    VEC3_TYPE sun_dir;
    bool debug_cascades;
};

// Same as VkDrawIndirectCommand
struct DrawIndirectCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct ShadowPassConstant {
    uint cascade_index;
};

struct MaterialInfo {
    uint32_t albedo_texture_index;
};
