#include "inputs/pos.hlsl"

[[vk::binding(0, 1)]] StructuredBuffer<float> vertices;
[[vk::binding(1, 1)]] StructuredBuffer<uint> indices;
[[vk::binding(2, 1)]] cbuffer m {
    float4x4 mm;
};
[[vk::binding(3, 1)]] StructuredBuffer<float> normals;


[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    uint offset = indices.Load(vId) * 3;
    float3 vertex = float3(vertices.Load(offset), vertices.Load(offset + 1), vertices.Load(offset + 2));

    float3 normal = float3(normals.Load(offset), normals.Load(offset + 1), normals.Load(offset + 2));

    V2P vsOut;
    vsOut.world_pos = normal;
    vsOut.Pos = mul(mm, float4(vertex, 1.0));
    return vsOut;
}

[shader("pixel")]
void PSMain(
    V2P psIn,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    target_0 = float4(psIn.world_pos, 1.0);
}
