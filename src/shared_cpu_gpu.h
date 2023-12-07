#ifndef GLSL
    #pragma once
using namespace glm;
#else
    #include "shaders/common/hlsl4glsl.glsl"
#endif

const static uint8_t MESH_INFO_FLAGS_32_BIT_INDICES = uint8_t(1 << 0);
const static uint8_t MESH_INFO_FLAGS_ALPHA_CLIP = uint8_t(1 << 1);

// Same as VkDispatchIndirectCommand.
struct DispatchIndirectCommand {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct MeshInfo {
    uint64_t positions;
    uint64_t indices;
    uint64_t normals;
    uint64_t uvs;
    uint64_t micro_indices;
    uint64_t meshlets;
    uint16_t num_meshlets;
    uint8_t flags;
    vec4 bounding_sphere;
    vec2 texture_scale;
    vec2 texture_offset;
    uint16_t base_color_texture_index;
    uint16_t metallic_roughness_texture_index;
    uint16_t normal_texture_index;
    vec3 base_color_factor;
};

struct MiscStorage {
    mat4 shadow_matrices[4];
    mat4 uv_space_shadow_matrices[4];
    mat4 shadow_view_matrices[4];
    float shadow_sphere_radii[4];
    uint32_t min_depth;
    uint32_t max_depth;
    DispatchIndirectCommand per_instance_dispatch;
    DispatchIndirectCommand per_meshlet_dispatch;
    DispatchIndirectCommand per_shadow_meshlet_dispatch;
};

// This is only an int32_t because of imgui.
const static int32_t UNIFORMS_DEBUG_OFF = 0;
const static int32_t UNIFORMS_DEBUG_CASCADES = 1;
const static int32_t UNIFORMS_DEBUG_TRIANGLE_INDEX = 2;
const static int32_t UNIFORMS_DEBUG_INSTANCE_INDEX = 3;
const static int32_t UNIFORMS_DEBUG_SHADER_CLOCK = 4;
const static int32_t UNIFORMS_DEBUG_NORMALS = 5;

struct Uniforms {
    mat4 combined_perspective_view;
    mat4 inv_perspective_view;
    mat4 view;
    mat4 initial_view;
    mat4 perspective;
    mat4 perspective_inverse;
    mat4 view_inverse;
    uint64_t meshlet_references;
    uint64_t instances;
    uint64_t draw_calls;
    uint64_t misc_storage;
    uint64_t num_meshlets_prefix_sum;
    vec3 camera_pos;
    vec3 sun_dir;
    vec3 sun_intensity;
    uint32_t num_instances;
    uvec2 window_size;
    float shadow_cam_distance;
    float cascade_split_pow;
    int32_t debug;
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
    mat4 transform;
    uint64_t mesh_info_address;
    mat3 normal_transform;

#ifndef GLSL
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

struct CalcBoundingSphereConstant {
    uint32_t num_vertices;
};

struct CopyQuantizedPositionsConstant {
    uint64_t dst;
    uint64_t src;
    uint32_t count;
};

struct UniformBufferAddressConstant {
    uint64_t address;
};

const static uint32_t MAX_OPAQUE_DRAWS = 200000;
const static uint32_t MAX_ALPHA_CLIP_DRAWS = 200000;

const static uint32_t ALPHA_CLIP_DRAWS_OFFSET = MAX_OPAQUE_DRAWS;

const static uint32_t MESHLET_INDICES_BUFFER_SECTION_OFFSET =
    ALPHA_CLIP_DRAWS_OFFSET + MAX_ALPHA_CLIP_DRAWS;

// sizeof(uint32_t4) * 2
const static uint32_t DRAW_CALLS_COUNTS_SIZE = 4 * 4 * 2;

const static float NEAR_PLANE = 0.01f;

struct Meshlet {
    vec3 cone_apex;
    vec3 cone_axis;
    float cone_cutoff;
    vec4 bounding_sphere;
    // The buffers these index into are often large enough to require 32-bit offsets.
    uint32_t triangle_offset;
    uint32_t index_offset;
    uint8_t triangle_count;
    uint8_t index_count;
};

// Allows for indexing a meshlet in the mesh that an instance represents.
struct MeshletReference {
    uint32_t instance_index;
    uint16_t meshlet_index;
};

// Defaults from https://github.com/zeux/meshoptimizer#mesh-shading
const static uint32_t MAX_MESHLET_UNIQUE_VERTICES = 64;
const static uint32_t MAX_MESHLET_TRIANGLES = 124;
const static uint32_t MAX_MESHLET_VERTICES = MAX_MESHLET_TRIANGLES * 3;

const static uint16_t UNUSED_TEXTURE_INDEX = ~uint16_t(0u);

struct NumMeshletsPrefixSumResult {
    uint32_t instance_index;
    uint32_t meshlets_offset;
};
