#include "inputs/pos_uv.hlsl"
#include "bindings.hlsl"

float3 tony_mc_mapface(float3 stimulus) {
    // Apply a non-linear transform that the LUT is encoded with.
    const float3 encoded = stimulus / (stimulus + 1.0);

    // Align the encoded range to texel centers.
    const float LUT_DIMS = 48.0;
    const float3 uv = encoded * ((LUT_DIMS - 1.0) / LUT_DIMS) + 0.5 / LUT_DIMS;

    // Note: for OpenGL, do `uv.y = 1.0 - uv.y`

    return display_transform_lut.SampleLevel(clamp_sampler, uv, 0);
}

[shader("pixel")]
void PSMain(
    V2P psIn,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    psIn.Uv.y = 1.0 - psIn.Uv.y;

    float3 stimulus = source_tex.Sample(clamp_sampler, psIn.Uv);
    target_0 = float4(stimulus, 1.0);

    float shadow_screen_percentage = 0.3;

    if (psIn.Uv.x < shadow_screen_percentage && psIn.Uv.y < shadow_screen_percentage) {
        psIn.Uv /= shadow_screen_percentage;
        target_0 = float4(shadowmap.Sample(clamp_sampler, psIn.Uv).xxx, 1.0);
    }
}
