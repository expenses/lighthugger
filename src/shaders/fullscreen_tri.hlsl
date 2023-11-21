#include "inputs/pos_uv.hlsl"

[shader("vertex")]
Varyings VSMain(uint vId : SV_VertexID)
{
    Varyings outputs;
    outputs.uv = float2((vId << 1) & 2, vId & 2);
    outputs.clip_pos = float4((2.0 * outputs.uv) - 1.0, 0.0, 1.0);
    return outputs;
}
