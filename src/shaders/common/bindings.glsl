#include <shared_cpu_gpu.h>

#include "buffer_references.glsl"

// Ensure that all of these are scalar!

layout(binding = 0) uniform texture2D textures[];

layout(binding = 1, scalar) uniform UniformsBinding {
    Uniforms uniforms;
};

layout(binding = 2) uniform texture2D scene_referred_framebuffer;
layout(binding = 3) uniform texture3D display_transform_lut;
layout(binding = 4) uniform texture2D depth_buffer;

layout(binding = 5) uniform texture2DArray shadowmap;
layout(binding = 6) uniform writeonly image2D rw_scene_referred_framebuffer;
layout(binding = 7) uniform utexture2D visibility_buffer;

layout(binding = 8) uniform sampler clamp_sampler;
layout(binding = 9) uniform sampler repeat_sampler;
layout(binding = 10) uniform sampler shadowmap_comparison_sampler;

uint32_t load_index(MeshInfo mesh_info, uint32_t vertex_id) {
    if (bool(mesh_info.flags & MESH_INFO_FLAGS_32_BIT_INDICES)) {
        return Index32Buffer(mesh_info.indices).indices[vertex_id];
    } else {
        return Index16Buffer(mesh_info.indices).indices[vertex_id];
    }
}

float3
calculate_world_pos(Instance instance, MeshInfo mesh_info, uint32_t index) {
    float3 position =
        QuantizedPositionBuffer(mesh_info.positions).positions[index];
    return (instance.transform * float4(position, 1.0)).xyz;
}
