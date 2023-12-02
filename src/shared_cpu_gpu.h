#ifndef GLSL
    #pragma once
using namespace glm;
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
    vec4 bounding_sphere;
    vec2 texture_scale;
    vec2 texture_offset;
    uint32_t albedo_texture_index;
    uint32_t metallic_roughness_texture_index;
    uint32_t normal_texture_index;
    vec3 albedo_factor;
};

// Stores depth info and draw call counts.
struct MiscStorage {
    mat4 shadow_matrices[4];
    uint32_t min_depth;
    uint32_t max_depth;
    uint32_t num_opaque_meshlets;
    uint32_t num_alpha_clip_meshlets;
    uint32_t num_expanded_meshlets;
    uint32_t _padding0;
    uint32_t _padding1;
    uint32_t _padding2;
};

// This is only an int32_t because of imgui.
const static int32_t UNIFORMS_DEBUG_OFF = 0;
const static int32_t UNIFORMS_DEBUG_CASCADES = 1;
const static int32_t UNIFORMS_DEBUG_TRIANGLE_INDEX = 2;
const static int32_t UNIFORMS_DEBUG_INSTANCE_INDEX = 3;
const static int32_t UNIFORMS_DEBUG_SHADER_CLOCK = 4;

struct Uniforms {
    mat4 combined_perspective_view;
    mat4 inv_perspective_view;
    mat4 view;
    mat4 initial_view;
    mat4 perspective;
    uint64_t instance_meshlets;
    uint64_t expanded_meshlets;
    vec3 camera_pos;
    uint32_t _padding0;
    vec3 sun_dir;
    uint32_t _padding1;
    vec3 sun_intensity;
    uint32_t num_instances;
    uvec2 window_size;
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
    vec3 cone_apex;
    vec3 cone_axis;
    float cone_cutoff;
    vec4 bounding_sphere;
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

const static uint32_t MAX_MESHLET_TRIANGLES = 124;
const static uint32_t MAX_MESHLET_VERTICES = MAX_MESHLET_TRIANGLES * 3;
