#pragma once
#include <shared_cpu_gpu.h>
#include "loading.hlsl"

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
[[vk::binding(8)]] RWStructuredBuffer<MiscStorageBuffer> misc_storage;
[[vk::binding(9)]] Texture2DArray<float> shadowmap;
[[vk::binding(10)]] SamplerComparisonState shadowmap_comparison_sampler;
[[vk::binding(11)]] RWStructuredBuffer<DrawIndirectCommand> draw_calls;

[[vk::binding(0, 1)]] RWTexture2D<float4> swapchain_image;

Instance load_instance(uint32_t offset) {
    uint32_t packed_instance_size = 112;
    offset = offset * packed_instance_size;
    Instance instance;
    instance.transform = load_value_and_increment_offset<float4x4>(instances, offset);
    instance.mesh_info_address = load_value_and_increment_offset<uint64_t>(instances, offset);
    instance.normal_transform = float3x3(
        load_value_and_increment_offset<float3>(instances, offset),
        load_value_and_increment_offset<float3>(instances, offset),
        load_value_and_increment_offset<float3>(instances, offset)
    );
    return instance;
}
