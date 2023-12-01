#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_buffer_reference: require
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_samplerless_texture_functions: require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_ARB_shader_clock: require

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4
#define uint32_t2 uvec2
#define uint32_t3 uvec3
#define uint32_t4 uvec4
#define uint3 uvec3
#define uint8_t3 u8vec3
#define uint16_t2 u16vec2
#define uint16_t3 u16vec3
#define uint16_t4 u16vec4
#define int8_t4 i8vec4

#define static

#define __HLSL_VERSION 1

float3 rcp(float3 value) {
    return 1.0 / value;
}

float rcp(float value) {
    return 1.0 / value;
}

float select(bool boolean, float true_value, float false_value) {
    return mix(false_value, true_value, boolean);
}

float3 select(bool boolean, float3 true_value, float3 false_value) {
    return mix(false_value, true_value, bvec3(boolean));
}
