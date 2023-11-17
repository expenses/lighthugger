#include "inputs/pos.hlsl"

#include "../shared_cpu_gpu.h"

[[vk::binding(0)]] StructuredBuffer<MeshBufferAddresses> mesh_buffer_addresses;
[[vk::binding(1)]] cbuffer uniforms {
    float4x4 combined_perspective_view_matrix;
};

float3 load_float3(uint64_t address, uint offset) {
    return float3(
        vk::RawBufferLoad<float>(address + sizeof(float) * offset),
        vk::RawBufferLoad<float>(address + sizeof(float) * (offset + 1)),
        vk::RawBufferLoad<float>(address + sizeof(float) * (offset + 2))
    );
}

[shader("vertex")]
float4 depth_only(
    uint vId : SV_VertexID
): SV_Position
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint offset = vk::RawBufferLoad<uint>(addresses.indices + sizeof(uint) * vId) * 3;
    float3 position = load_float3(addresses.positions, offset);

    return mul(combined_perspective_view_matrix, float4(position, 1.0));
}

[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint offset = vk::RawBufferLoad<uint>(addresses.indices + sizeof(uint) * vId) * 3;
    float3 position = load_float3(addresses.positions, offset);
    float3 normal = load_float3(addresses.normals, offset);

    V2P vsOut;
    vsOut.world_pos = normal;
    vsOut.Pos = mul(combined_perspective_view_matrix, float4(position, 1.0));
    vsOut.normal = normal;
    return vsOut;
}

[shader("pixel")]
void PSMain(
    V2P psIn,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    float3 sun_dir = normalize(float3(1, 2, 1));
    float3 normal = normalize(psIn.normal);

    float sun_factor = max(dot(sun_dir, normal), 0.0);
    float3 irradiance = sun_factor;

    target_0 = float4(irradiance, 1.0);
}
