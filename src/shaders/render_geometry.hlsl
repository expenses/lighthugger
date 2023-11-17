#include "inputs/pos.hlsl"

#include "../shared_cpu_gpu.h"

[[vk::binding(0)]] StructuredBuffer<MeshBufferAddresses> mesh_buffer_addresses;
[[vk::binding(1)]] cbuffer uniforms {
    float4x4 combined_perspective_view_matrix;
};
[[vk::binding(5)]] SamplerState samp;
[[vk::binding(6)]] Texture2D textures[];

float3 load_float3(uint64_t address, uint offset) {
    return float3(
        vk::RawBufferLoad<float>(address + sizeof(float) * (offset * 3)),
        vk::RawBufferLoad<float>(address + sizeof(float) * ((offset * 3) + 1)),
        vk::RawBufferLoad<float>(address + sizeof(float) * ((offset * 3) + 2))
    );
}

[shader("vertex")]
float4 depth_only(
    uint vId : SV_VertexID
): SV_Position
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint offset = vk::RawBufferLoad<uint>(addresses.indices + sizeof(uint) * vId);
    float3 position = load_float3(addresses.positions, offset);

    return mul(combined_perspective_view_matrix, float4(position, 1.0));
}

[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint material_index = vk::RawBufferLoad<uint>(addresses.material_indices + sizeof(uint) * vId);
    uint offset = vk::RawBufferLoad<uint>(addresses.indices + sizeof(uint) * vId);
    float3 position = load_float3(addresses.positions, offset);
    float3 normal = load_float3(addresses.normals, offset);
    float2 uv = vk::RawBufferLoad<float2>(addresses.uvs + sizeof(float2) * offset);

    V2P vsOut;
    vsOut.world_pos = normal;
    vsOut.Pos = mul(combined_perspective_view_matrix, float4(position, 1.0));
    vsOut.normal = normal;
    vsOut.material_index = material_index;
    vsOut.uv = uv;
    return vsOut;
}

const float3 c[10] = {
    float3(   0.0f/255.0f,   2.0f/255.0f,  91.0f/255.0f ),
    float3(   0.0f/255.0f, 108.0f/255.0f, 251.0f/255.0f ),
    float3(   0.0f/255.0f, 221.0f/255.0f, 221.0f/255.0f ),
    float3(  51.0f/255.0f, 221.0f/255.0f,   0.0f/255.0f ),
    float3( 255.0f/255.0f, 252.0f/255.0f,   0.0f/255.0f ),
    float3( 255.0f/255.0f, 180.0f/255.0f,   0.0f/255.0f ),
    float3( 255.0f/255.0f, 104.0f/255.0f,   0.0f/255.0f ),
    float3( 226.0f/255.0f,  22.0f/255.0f,   0.0f/255.0f ),
    float3( 191.0f/255.0f,   0.0f/255.0f,  83.0f/255.0f ),
    float3( 145.0f/255.0f,   0.0f/255.0f,  65.0f/255.0f )
};

[shader("pixel")]
void PSMain(
    V2P input,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {

    float3 sun_dir = normalize(float3(1, 2, 1));
    float3 normal = normalize(input.normal);

    float sun_factor = max(dot(sun_dir, normal), 0.0);
    float3 albedo = textures[input.material_index].Sample(samp, input.uv).rgb;


    target_0 = float4(albedo * sun_factor + albedo * 0.025, 1.0);
}
