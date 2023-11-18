#include "inputs/pos.hlsl"
#include "bindings.hlsl"

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

    return mul(uniforms.combined_perspective_view, float4(position, 1.0));
}

[shader("vertex")]
float4 shadow_pass(
    uint vId : SV_VertexID
): SV_Position
{
    MeshBufferAddresses addresses = mesh_buffer_addresses.Load(0);

    uint offset = vk::RawBufferLoad<uint>(addresses.indices + sizeof(uint) * vId);
    float3 position = load_float3(addresses.positions, offset);

    return mul(depth_info[0].shadow_rendering_matrices[3], float4(position, 1.0));
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
    float4 shadowCoord = mul(biasMat, mul(depth_info[0].shadow_rendering_matrices[3], float4(input.world_pos, 1.0)));
    float shadow = textureProj(shadowCoord / shadowCoord.w, float2(0.0, 0.0), 2);

    float3 normal = normalize(input.normal);

    float sun_factor = max(dot(SUN_DIR, normal), 0.0) * shadow;
    float3 albedo = textures[input.material_index].Sample(repeat_sampler, input.uv).rgb;

    float ambient = 0.0;

    target_0 = float4(albedo * (sun_factor + ambient), 1.0);
}
