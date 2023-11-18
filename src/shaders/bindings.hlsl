#include "../shared_cpu_gpu.h"

static float3 SUN_DIR = normalize(float3(1, 2, 1));

[[vk::binding(0)]] StructuredBuffer<MeshBufferAddresses> mesh_buffer_addresses;
[[vk::binding(1)]] cbuffer uniforms {
    Uniforms uniforms;
};
[[vk::binding(2)]] Texture2D<float3> source_tex;
[[vk::binding(3)]] SamplerState clamp_sampler;
[[vk::binding(4)]] Texture3D<float3> display_transform_lut;
[[vk::binding(5)]] SamplerState repeat_sampler;
[[vk::binding(6)]] Texture2D<float> depth_buffer;
[[vk::binding(7)]] RWStructuredBuffer<DepthInfoBuffer> depth_info;
[[vk::binding(8)]] Texture2D<float> shadowmap;
[[vk::binding(9)]] Texture2D textures[];
