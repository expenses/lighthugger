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
    uint32_t num_indices;
    float bounding_sphere_radius;
};

struct DepthInfoBuffer {
    uint32_t min_depth;
    uint32_t max_depth;
    MATRIX_TYPE shadow_rendering_matrices[4];
    //float cascade_splits[4];
};

struct Uniforms {
    MATRIX_TYPE combined_perspective_view;
    MATRIX_TYPE inv_perspective_view;
    bool stabilize_cascades;
    VEC3_TYPE sun_dir;
};
