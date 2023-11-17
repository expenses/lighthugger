#include "inputs/pos.hlsl"

[[vk::binding(0, 1)]] StructuredBuffer<float> vertices;
[[vk::binding(1, 1)]] StructuredBuffer<uint> indices;
[[vk::binding(2, 1)]] cbuffer m {
    float4x4 mm;
};

[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    uint index = indices.Load(vId);

    V2P vsOut;
    float3 vertex = float3(vertices.Load(index * 3), vertices.Load(index * 3 + 1), vertices.Load(index * 3 + 2));
    vsOut.world_pos = vertex;
    vsOut.Pos = float4((vertex + float3(-115.0, 0.0, 0.0)).xy * 0.1, 0.0, 1.0);//mul(mm, float4(vertex, 1.0));
    return vsOut;
}

[shader("pixel")]
void PSMain(
    V2P psIn,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    target_0 = float4(psIn.world_pos, 1.0);
}
