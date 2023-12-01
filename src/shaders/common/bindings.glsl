#include "hlsl4glsl.glsl"
#include <shared_cpu_gpu.h>

// Ensure that all of these are scalar!

layout(binding = 0) uniform texture2D textures[];

layout(binding = 1, scalar) buffer InstanceBuffer {
    Instance instances[];
};

layout(binding = 2, scalar) uniform UniformsBinding {
    Uniforms uniforms;
};

layout(binding = 4) uniform sampler clamp_sampler;

layout(binding = 6) uniform sampler repeat_sampler;

layout(binding = 7) uniform texture2D depth_buffer;

layout(binding = 8, scalar) buffer MiscStorageBuffer {
    MiscStorage misc_storage;
};

layout(binding = 9) uniform texture2DArray shadowmap;

layout(binding = 10) uniform sampler shadowmap_comparison_sampler;

layout(binding = 11, scalar) buffer DrawCallBuffer {
    DrawIndirectCommand draw_calls[];
};

layout(binding = 12) uniform writeonly image2D rw_scene_referred_framebuffer;

layout(binding = 13) uniform utexture2D visibility_buffer;


layout(buffer_reference, scalar) buffer MeshInfoBuffer {
    MeshInfo mesh_info;
};

layout(buffer_reference, scalar) buffer MeshletIndexBuffer {
    MeshletIndex meshlet_index[];
};

layout(buffer_reference, scalar) buffer MeshletBuffer {
    Meshlet meshlets[];
};

layout(buffer_reference, scalar, buffer_reference_align = 1) buffer MicroIndexBuffer {
    u8vec3 indices[];
};

layout(buffer_reference, scalar) buffer Index32Buffer {
    uint32_t indices[];
};

layout(buffer_reference, scalar, buffer_reference_align = 2) buffer Index16Buffer {
    uint16_t indices[];
};

uint32_t load_index(MeshInfo mesh_info, uint32_t vertex_id) {
    if (bool(mesh_info.flags & MESH_INFO_FLAGS_32_BIT_INDICES)) {
        return Index32Buffer(mesh_info.indices).indices[vertex_id];
    } else {
        return Index16Buffer(mesh_info.indices).indices[vertex_id];
    }
}

layout(buffer_reference, scalar, buffer_reference_align = 2) buffer QuantizedPositionBuffer {
    uint16_t3 positions[];
};

layout(buffer_reference, scalar) buffer QuanitizedUvs {
    uint16_t2 uvs[];
};

layout(buffer_reference, scalar) buffer QuanitizedNormals {
    int8_t4 normals[];
};


float3 calculate_world_pos(
    Instance instance,
    MeshInfo mesh_info,
    uint32_t index
) {
    float3 position = QuantizedPositionBuffer(mesh_info.positions).positions[index];
    return (instance.transform * float4(position, 1.0)).xyz;
}
