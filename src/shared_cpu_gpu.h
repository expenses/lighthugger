#pragma once

#ifdef __HLSL_VERSION
    #define MATRIX4_TYPE float4x4
    #define MATRIX3_TYPE float3x3
    #define VEC3_TYPE float3
#else
    #define MATRIX4_TYPE glm::mat4
    #define MATRIX3_TYPE glm::mat3
    #define VEC3_TYPE glm::vec3
#endif

struct MeshInfo {
    uint64_t positions;
    uint64_t indices;
    uint64_t normals;
    uint64_t uvs;
    uint64_t material_indices;
    uint64_t material_info;
    uint32_t num_indices;
    float bounding_sphere_radius;
};

struct DepthInfoBuffer {
    MATRIX4_TYPE shadow_rendering_matrices[4];
    uint32_t min_depth;
    uint32_t max_depth;
};

struct Uniforms {
    MATRIX4_TYPE combined_perspective_view;
    MATRIX4_TYPE inv_perspective_view;
    MATRIX4_TYPE view;
    VEC3_TYPE sun_dir;
    uint32_t _padding0;
    VEC3_TYPE sun_intensity;
    uint32_t num_instances;
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
    MeshInfo mesh_info;
    MATRIX3_TYPE normal_transform;

#ifndef __HLSL_VERSION
    Instance(glm::mat4 transform_, MeshInfo mesh_info_) :
        transform(transform_),
        mesh_info(mesh_info_) {
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

struct MaterialInfo {
    uint32_t albedo_texture_index;
};
