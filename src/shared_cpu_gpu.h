#pragma once

#ifdef __HLSL_VERSION
    #define MATRIX4_TYPE float4x4
    #define MATRIX3_TYPE float3x3
    #define VEC3_TYPE float3
    #define VEC2_TYPE float2
    #define UVEC2_TYPE uint32_t2
#else
    #define MATRIX4_TYPE glm::mat4
    #define MATRIX3_TYPE glm::mat3
    #define VEC3_TYPE glm::vec3
    #define VEC2_TYPE glm::vec2
    #define UVEC2_TYPE glm::uvec2
#endif

const static uint32_t MESH_INFO_FLAGS_32_BIT_INDICES = 1 << 0;
const static uint32_t MESH_INFO_FLAGS_ALPHA_CLIP = 1 << 1;

struct MeshInfo {
    uint64_t positions;
    uint64_t indices;
    uint64_t normals;
    uint64_t uvs;
    uint32_t num_indices;
    uint32_t num_vertices;
    uint32_t flags;
    float bounding_sphere_radius;
    VEC2_TYPE texture_scale;
    VEC2_TYPE texture_offset;
    uint32_t albedo_texture_index;
    uint32_t metallic_roughness_texture_index;
};

struct MeshInfoWithUintBoundingSphereRadius {
    uint64_t positions;
    uint64_t indices;
    uint64_t normals;
    uint64_t uvs;
    uint32_t num_indices;
    uint32_t num_vertices;
    uint32_t flags;
    uint32_t bounding_sphere_radius;
    VEC2_TYPE texture_scale;
    VEC2_TYPE texture_offset;
    uint32_t albedo_texture_index;
    uint32_t metallic_roughness_texture_index;
};

// Stores depth info and draw call counts.
struct MiscStorageBuffer {
    MATRIX4_TYPE shadow_rendering_matrices[4];
    uint32_t min_depth;
    uint32_t max_depth;
    uint32_t num_opaque_draws;
    uint32_t num_alpha_clip_draws;
};

struct Uniforms {
    MATRIX4_TYPE combined_perspective_view;
    MATRIX4_TYPE inv_perspective_view;
    MATRIX4_TYPE view;
    VEC3_TYPE sun_dir;
    uint32_t _padding0;
    VEC3_TYPE sun_intensity;
    uint32_t num_instances;
    UVEC2_TYPE window_size;
    bool debug_cascades;
    uint32_t _padding;
    bool debug_shadowmaps;
};

// Same as VkDrawIndirectCommand
struct DrawIndirectCommand {
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
    uint32_t firstInstance;
};

struct Instance {
    MATRIX4_TYPE transform;
    uint64_t mesh_info_address;
    MATRIX3_TYPE normal_transform;

#ifndef __HLSL_VERSION
    Instance(glm::mat4 transform_, uint64_t mesh_info_address_) :
        transform(transform_),
        mesh_info_address(mesh_info_address_) {
        // Normally you want to do a transpose for this but for hlsl you don't seem to need to. Not sure why.
        normal_transform = glm::mat3(glm::inverse(transform));
    }
#endif
};

struct ShadowPassConstant {
    uint32_t cascade_index;
};

struct DisplayTransformConstant {
    uint32_t swapchain_image_index;
};

struct CopyQuantizedPositionsConstant {
    uint64_t dst;
    uint64_t src;
    uint32_t count;
};
