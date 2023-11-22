#include "../shared_cpu_gpu.h"

[[vk::binding(0)]] Texture2D textures[];
[[vk::binding(1)]] ByteAddressBuffer instances;
[[vk::binding(2)]] cbuffer uniforms {
    Uniforms uniforms;
}
[[vk::binding(3)]] Texture2D<float3> scene_referred_framebuffer;
[[vk::binding(4)]] SamplerState clamp_sampler;
[[vk::binding(5)]] Texture3D<float3> display_transform_lut;
[[vk::binding(6)]] SamplerState repeat_sampler;
[[vk::binding(7)]] Texture2D<float> depth_buffer;
[[vk::binding(8)]] RWStructuredBuffer<DepthInfoBuffer> depth_info;
[[vk::binding(9)]] Texture2DArray<float> shadowmap;
[[vk::binding(10)]] SamplerComparisonState shadowmap_comparison_sampler;
[[vk::binding(11)]] RWStructuredBuffer<DrawIndirectCommand> draw_calls;
[[vk::binding(12)]] RWStructuredBuffer<uint32_t> draw_counts;
[[vk::binding(13)]] RWTexture2D<float4> swapchain_images[];

Instance load_instance(uint32_t offset) {
    uint32_t packed_instance_size = 160;
    offset = offset * packed_instance_size;
    Instance instance;// = instances.Load<Instance>(base);
    instance.transform = instances.Load<float4x4>(offset);
    offset += sizeof(float4x4);
    instance.mesh_info = instances.Load<MeshInfo>(offset);
    offset += sizeof(MeshInfo);
    instance.normal_transform = float3x3(
        instances.Load<float3>(offset),
        instances.Load<float3>(offset + sizeof(float3)),
        instances.Load<float3>(offset + sizeof(float3) * 2)
    );
    return instance;
}
