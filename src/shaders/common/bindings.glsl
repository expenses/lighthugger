#include <shared_cpu_gpu.h>

#include "buffer_references.glsl"

// Ensure that all of these are scalar!

layout(binding = 0) uniform texture2D textures[];

layout(binding = 1) uniform texture2D scene_referred_framebuffer;
layout(binding = 2) uniform texture3D display_transform_lut;
layout(binding = 3) uniform texture2D depth_buffer;

layout(binding = 4) uniform texture2DArray shadowmap;
layout(binding = 5) uniform writeonly image2D rw_scene_referred_framebuffer;
layout(binding = 6) uniform utexture2D visibility_buffer;

layout(binding = 7) uniform sampler clamp_sampler;
layout(binding = 8) uniform sampler repeat_sampler;
layout(binding = 9) uniform sampler shadowmap_comparison_sampler;

layout(binding = 10) uniform textureCube skybox;

layout(push_constant) uniform UniformPushConstant {
    UniformBufferAddressConstant uniform_buffer;
    ShadowPassConstant shadow_constant;
};

Uniforms get_uniforms() {
    return UniformsBuffer(uniform_buffer.address).uniforms;
}
