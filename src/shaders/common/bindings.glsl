#include <shared_cpu_gpu.h>

#include "buffer_references.glsl"

// Ensure that all of these are scalar!

layout(binding = 0) uniform texture2D textures[];

layout(binding = 1, scalar) uniform UniformsBinding {
    Uniforms uniforms;
};

layout(binding = 2) uniform texture2D scene_referred_framebuffer;
layout(binding = 3) uniform texture3D display_transform_lut;
layout(binding = 4) uniform texture2D depth_buffer;

layout(binding = 5) uniform texture2DArray shadowmap;
layout(binding = 6) uniform writeonly image2D rw_scene_referred_framebuffer;
layout(binding = 7) uniform utexture2D visibility_buffer;

layout(binding = 8) uniform sampler clamp_sampler;
layout(binding = 9) uniform sampler repeat_sampler;
layout(binding = 10) uniform sampler shadowmap_comparison_sampler;

layout(binding = 11) uniform textureCube skybox;
