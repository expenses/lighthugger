#include "common/bindings.glsl"

layout(set = 1, binding = 0) uniform writeonly image2D swapchain_image;

float3 tony_mc_mapface(float3 stimulus) {
    // Apply a non-linear transform that the LUT is encoded with.
    const float3 encoded = stimulus / (stimulus + 1.0);

    // Align the encoded range to texel centers.
    const float LUT_DIMS = 48.0;
    const float3 uv = encoded * ((LUT_DIMS - 1.0) / LUT_DIMS) + 0.5 / LUT_DIMS;

    // Note: for OpenGL, do `uv.y = 1.0 - uv.y`

    return textureLod(sampler3D(display_transform_lut, clamp_sampler), uv, 0.0).xyz;
}

float linear_to_srgb_transfer_function(float value) {
    return select(value <= 0.003130, value * 12.92, 1.055 * pow(value, 1.0/2.4) - 0.055);
}

float3 linear_to_srgb_transfer_function(float3 value) {
    return float3(linear_to_srgb_transfer_function(value.x), linear_to_srgb_transfer_function(value.y), linear_to_srgb_transfer_function(value.z));
}

layout(push_constant) uniform PushConstant {
    DisplayTransformConstant display_transform_constant;
};

layout(local_size_x = 8, local_size_y = 8) in;

//comp
void display_transform() {
    if (gl_GlobalInvocationID.x >= uniforms.window_size.x || gl_GlobalInvocationID.y >= uniforms.window_size.y) {
        return;
    }

    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);

    float3 stimulus = texelFetch(scene_referred_framebuffer, coord, 0).rgb;

    float3 linear_display_referred_value = tony_mc_mapface(stimulus);

    if (uniforms.debug_shadowmaps) {
        float2 uv = (float2(gl_GlobalInvocationID.xy) + 0.5) / float2(uniforms.window_size);

        float shadow_screen_percentage = 0.3;

        if (uv.x < shadow_screen_percentage && uv.y < shadow_screen_percentage) {
            float2 shadow_uv = uv / shadow_screen_percentage;
            shadow_uv.x = 1.0 - shadow_uv.x;
            linear_display_referred_value = textureLod(sampler2DArray(shadowmap, clamp_sampler), float3(shadow_uv, 0), 0.0).xxx;
        }
    }

    float3 srgb_encoded_value = linear_to_srgb_transfer_function(linear_display_referred_value);

    imageStore(swapchain_image, coord, float4(srgb_encoded_value, 1.0));
}
