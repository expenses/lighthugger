
struct V2P
{
    float4 Pos : SV_Position;
    float2 Uv : COLOR0;
};

[shader("vertex")]
V2P VSMain(uint vId : SV_VertexID)
{
    float2 uv = float2((vId << 1) & 2, vId & 2);
    V2P vsOut;
    vsOut.Uv = float2(uv.x, 1.0 - uv.y);
    vsOut.Pos = float4((2.0 * uv) - 1.0, 0.0, 1.0);
    return vsOut;
}

[[vk::binding(0)]] Texture2D<float3> source_tex;
[[vk::binding(1)]] SamplerState samp;
[[vk::binding(2)]] Texture3D<float3> display_transform_lut;
[[vk::binding(3)]] StructuredBuffer<float3> vertices;

float3 tony_mc_mapface(float3 stimulus) {
    // Apply a non-linear transform that the LUT is encoded with.
    const float3 encoded = stimulus / (stimulus + 1.0);

    // Align the encoded range to texel centers.
    const float LUT_DIMS = 48.0;
    const float3 uv = encoded * ((LUT_DIMS - 1.0) / LUT_DIMS) + 0.5 / LUT_DIMS;

    // Note: for OpenGL, do `uv.y = 1.0 - uv.y`

    return display_transform_lut.SampleLevel(samp, uv, 0);
}

[shader("pixel")]
float4 PSMain(V2P psIn) : SV_Target0
{
    float3 stimulus = source_tex.Sample(samp, psIn.Uv);


    return float4(vertices.Load(0), 1.0);
}
