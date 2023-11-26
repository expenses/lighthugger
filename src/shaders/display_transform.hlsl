#include "common/bindings.hlsl"

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

[[vk::push_constant]]
DisplayTransformConstant display_transform_constant;

[shader("compute")]
[numthreads(8, 8, 1)]
void display_transform(
    uint3 global_id: SV_DispatchThreadID
) {
    if (global_id.x >= uniforms.window_size.x || global_id.y >= uniforms.window_size.y) {
        return;
    }

    float3 stimulus = scene_referred_framebuffer[global_id.xy];

    float3 linear_display_referred_value = tony_mc_mapface(stimulus);

    if (uniforms.debug_shadowmaps) {
        float2 uv = (float2(global_id.xy) + 0.5) / float2(uniforms.window_size);

        float shadow_screen_percentage = 0.3;

        if (uv.x < shadow_screen_percentage && uv.y < shadow_screen_percentage) {
            float2 shadow_uv = uv / shadow_screen_percentage;
            shadow_uv.x = 1.0 - shadow_uv.x;
            linear_display_referred_value = shadowmap.SampleLevel(clamp_sampler, float3(shadow_uv, 0), 0.0).xxx;
        }
    }

    float3 srgb_encoded_value = linear_to_srgb_transfer_function(linear_display_referred_value);

    swapchain_image[global_id.xy] = float4(srgb_encoded_value, 1.0);
}
