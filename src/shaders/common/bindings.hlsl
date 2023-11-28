#pragma once
#include <shared_cpu_gpu.h>
#include "loading.hlsl"

[[vk::binding(0)]] Texture2D textures[];
[[vk::binding(1)]] ByteAddressBuffer instances;
[[vk::binding(2)]] cbuffer uniforms {
    Uniforms uniforms;
}
// We could only use `rw_scene_referred_framebuffer` for this,
// but I'm saving this binding for when I need to be able
// to do samples on the framebuffer for e.g. bloom.
[[vk::binding(3)]] Texture2D<float3> scene_referred_framebuffer;
[[vk::binding(4)]] SamplerState clamp_sampler;
[[vk::binding(5)]] Texture3D<float3> display_transform_lut;
[[vk::binding(6)]] SamplerState repeat_sampler;
[[vk::binding(7)]] Texture2D<float> depth_buffer;
[[vk::binding(8)]] RWStructuredBuffer<MiscStorageBuffer> misc_storage;
[[vk::binding(9)]] Texture2DArray<float> shadowmap;
[[vk::binding(10)]] SamplerComparisonState shadowmap_comparison_sampler;
[[vk::binding(11)]] RWStructuredBuffer<DrawIndirectCommand> draw_calls;
[[vk::binding(12)]] RWTexture2D<float4> rw_scene_referred_framebuffer;
[[vk::binding(13)]] Texture2D<uint32_t> visibility_buffer;
