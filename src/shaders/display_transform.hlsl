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

float linear_to_srgb_transfer_function(float value) {
    return value <= 0.003130 ? value * 12.92 : 1.055 * pow(value, 1.0/2.4) - 0.055;
}

float3 linear_to_srgb_transfer_function(float3 value) {
    return float3(linear_to_srgb_transfer_function(value.x), linear_to_srgb_transfer_function(value.y), linear_to_srgb_transfer_function(value.z));
}

[shader("pixel")]
void PSMain(
    Varyings inputs,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    float3 stimulus = scene_referred_framebuffer.Sample(clamp_sampler, inputs.uv);

    target_0 = float4(tony_mc_mapface(stimulus), 1.0);

    if (uniforms.debug_shadowmaps) {
        float shadow_screen_percentage = 0.3;

        if (inputs.uv.x < shadow_screen_percentage && inputs.uv.y < shadow_screen_percentage) {
            float2 shadow_uv = inputs.uv / shadow_screen_percentage;
            shadow_uv.x = 1.0 - shadow_uv.x;
            target_0 = float4(shadowmap.Sample(clamp_sampler, float3(shadow_uv, 0)).xxx, 1.0);
        }
    }

    target_0.xyz = linear_to_srgb_transfer_function(target_0.xyz);
}
