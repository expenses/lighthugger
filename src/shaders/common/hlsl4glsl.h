#extension GL_EXT_shader_explicit_arithmetic_types: require


#define float3x3 mat3
#define float3 vec3
#define float2 vec2
#define uint32_t2 uvec2
#define float4x4 mat4

#define static

#define __HLSL_VERSION 1

#define uint16_t4 u16vec4

#define global_id gl_GlobalInvocationID

#define vk::RawBufferLoad<uint16_t4> uint16_t4
