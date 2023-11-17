#include "inputs/pos_uv.hlsl"

[[vk::binding(0, 0)]] Texture2D<float3> source_tex;
[[vk::binding(1, 0)]] SamplerState samp;
[[vk::binding(2, 0)]] Texture3D<float3> display_transform_lut;

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
void PSMain(
    V2P psIn,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    psIn.Uv.y = 1.0 - psIn.Uv.y;
    float3 stimulus = source_tex.Sample(samp, psIn.Uv);
    target_0 = float4(stimulus, 1.0);
}
