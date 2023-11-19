#include "../shared_cpu_gpu.h"

static float3 SUN_DIR = normalize(float3(1, 2, 1));

[[vk::binding(0)]] Texture2D textures[];
[[vk::binding(1)]] StructuredBuffer<MeshBufferAddresses> mesh_buffer_addresses;
[[vk::binding(2)]] cbuffer uniforms {
    Uniforms uniforms;
};
[[vk::binding(3)]] Texture2D<float3> scene_referred_framebuffer;
[[vk::binding(4)]] SamplerState clamp_sampler;
[[vk::binding(5)]] Texture3D<float3> display_transform_lut;
[[vk::binding(6)]] SamplerState repeat_sampler;
[[vk::binding(7)]] Texture2D<float> depth_buffer;
[[vk::binding(8)]] RWStructuredBuffer<DepthInfoBuffer> depth_info;
[[vk::binding(9)]] Texture2D<float> shadowmap;
[[vk::binding(10)]] SamplerComparisonState shadowmap_comparison_sampler;
