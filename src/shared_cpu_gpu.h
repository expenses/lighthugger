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
    uint64_t micro_indices;
    uint64_t meshlets;
    uint32_t num_meshlets;
    uint32_t num_indices;
    uint32_t flags;
    float bounding_sphere_radius;
    VEC2_TYPE texture_scale;
    VEC2_TYPE texture_offset;
    uint32_t albedo_texture_index;
    uint32_t metallic_roughness_texture_index;
    uint32_t normal_texture_index;
    VEC3_TYPE albedo_factor;
};

struct MeshInfoWithUintBoundingSphereRadius {
    uint64_t positions;
    uint64_t indices;
    uint64_t normals;
    uint64_t uvs;
    uint64_t micro_indices;
    uint64_t meshlets;
    uint32_t num_meshlets;
    uint32_t num_indices;
    uint32_t flags;
    uint32_t bounding_sphere_radius;
    VEC2_TYPE texture_scale;
    VEC2_TYPE texture_offset;
    uint32_t albedo_texture_index;
    uint32_t metallic_roughness_texture_index;
    uint32_t normal_texture_index;
    VEC3_TYPE albedo_factor;
};

// Stores depth info and draw call counts.
struct MiscStorageBuffer {
    MATRIX4_TYPE shadow_matrices[4];
    uint32_t min_depth;
    uint32_t max_depth;
    uint32_t num_opaque_draws;
    uint32_t num_alpha_clip_draws;
};

// This is only an int32_t because of imgui.
const static int32_t UNIFORMS_DEBUG_OFF = 0;
const static int32_t UNIFORMS_DEBUG_CASCADES = 1;
const static int32_t UNIFORMS_DEBUG_TRIANGLE_INDEX = 2;
const static int32_t UNIFORMS_DEBUG_INSTANCE_INDEX = 3;
const static int32_t UNIFORMS_DEBUG_SHADER_CLOCK = 4;

struct Uniforms {
    MATRIX4_TYPE combined_perspective_view;
    MATRIX4_TYPE inv_perspective_view;
    MATRIX4_TYPE view;
    uint64_t instance_meshlets;
    VEC3_TYPE camera_pos;
    uint32_t _padding0;
    VEC3_TYPE sun_dir;
    uint32_t _padding1;
    VEC3_TYPE sun_intensity;
    uint32_t num_instances;
    UVEC2_TYPE window_size;
    float shadow_cam_distance;
    float cascade_split_pow;
    int32_t debug;
    uint32_t _padding2;
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

struct CalcBoundingSphereConstant {
    uint32_t num_vertices;
};

struct CopyQuantizedPositionsConstant {
    uint64_t dst;
    uint64_t src;
    uint32_t count;
};

const static uint32_t MAX_OPAQUE_DRAWS = 10000;
const static uint32_t MAX_ALPHA_CLIP_DRAWS = 2048;

const static uint32_t ALPHA_CLIP_DRAWS_OFFSET = MAX_OPAQUE_DRAWS;

const static float NEAR_PLANE = 0.01f;

struct Meshlet {
    VEC3_TYPE cone_apex;
    VEC3_TYPE cone_axis;
    float cone_cutoff;
    // The buffers these index into are often large enough to require 32-bit offsets.
    uint32_t triangle_offset;
    uint32_t index_offset;
    // hlsl doesn't support 8-bit types so we just pack these two
    // together.
    uint16_t triangle_count;
    uint16_t index_count;
};

struct MeshletIndex {
    uint32_t instance_index;
    uint32_t meshlet_index;
};
