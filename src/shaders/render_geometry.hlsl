#include "inputs/pos.hlsl"
#include "bindings.hlsl"

uint load_index(uint64_t address, uint vertex_id) {
    return vk::RawBufferLoad<uint>(address + sizeof(uint) * vertex_id);
}

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

    uint offset = load_index(addresses.indices, vId);
    float3 position = load_float3(addresses.positions, offset);

    return mul(uniforms.combined_perspective_view, float4(position, 1.0));
}

[shader("vertex")]
float4 shadow_pass(
    uint vId : SV_VertexID
): SV_Position
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint offset = load_index(addresses.indices, vId);
    float3 position = load_float3(addresses.positions, offset);

    return mul(depth_info[0].shadow_rendering_matrices[0], float4(position, 1.0));
}

[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint material_index = vk::RawBufferLoad<uint>(addresses.material_indices + sizeof(uint) * vId);
    uint offset = load_index(addresses.indices, vId);
    float3 position = load_float3(addresses.positions, offset);
    float3 normal = load_float3(addresses.normals, offset);
    float2 uv = vk::RawBufferLoad<float2>(addresses.uvs + sizeof(float2) * offset);

    V2P vsOut;
    vsOut.world_pos = position;
    vsOut.Pos = mul(uniforms.combined_perspective_view, float4(position, 1.0));
    vsOut.normal = normal;
    vsOut.material_index = material_index;
    vsOut.uv = uv;
    return vsOut;
}

// From the sascha willems tut.
float textureProj(float4 shadowCoord, float2 offset, uint cascadeIndex)
{
    float shadow = 1.0;
    float bias = 0.005;

    if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) {
            float dist = shadowmap.Sample(clamp_sampler, float2(shadowCoord.xy + offset));
            if (shadowCoord.w > 0 && dist < shadowCoord.z - bias) {
                    shadow = 0.0;
            }
    }
    return shadow;
}

static const float4x4 biasMat = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
);

[shader("pixel")]
void PSMain(
    V2P input,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    float4 shadowCoord = mul(biasMat, mul(depth_info[0].shadow_rendering_matrices[0], float4(input.world_pos, 1.0)));
    shadowCoord /= shadowCoord.w;
    float bias = 0.05;
    float shadow_sum = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) / 1024.0;
            shadow_sum += shadowmap.SampleCmpLevelZero(shadowmap_comparison_sampler, float2(shadowCoord.xy) + offset, shadowCoord.z - bias);
        }
    }
    shadow_sum /= 9.0;


    float3 normal = normalize(input.normal);

    float n_dot_l = max(dot(uniforms.sun_dir, normal), 0.0);
    float3 albedo = textures[input.material_index].Sample(repeat_sampler, input.uv).rgb;

    float ambient = 0.05;

    float3 lighting = max(n_dot_l * shadow_sum, ambient);

    float3 diffuse = albedo * lighting;

    target_0 = float4(diffuse, 1.0);
}
