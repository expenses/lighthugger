#include "inputs/pos.hlsl"

[[vk::binding(0, 1)]] StructuredBuffer<float> positions;
[[vk::binding(1, 1)]] StructuredBuffer<uint> indices;
[[vk::binding(2, 1)]] cbuffer uniforms {
    float4x4 combined_perspective_view_matrix;
};
[[vk::binding(3, 1)]] StructuredBuffer<float> normals;


[shader("vertex")]
float4 depth_only(
    uint vId : SV_VertexID
): SV_Position
{
    uint offset = indices.Load(vId) * 3;
    float3 position = float3(
        positions.Load(offset),
        positions.Load(offset + 1),
        positions.Load(offset + 2)
    );

    return mul(combined_perspective_view_matrix, float4(position, 1.0));
}


[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    uint offset = indices.Load(vId) * 3;
    float3 position = float3(
        positions.Load(offset),
        positions.Load(offset + 1),
        positions.Load(offset + 2)
    );

    float3 normal = float3(normals.Load(offset), normals.Load(offset + 1), normals.Load(offset + 2));

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
