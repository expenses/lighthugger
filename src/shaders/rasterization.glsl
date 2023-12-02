#include "common/bindings.glsl"

uint32_t pack(uint32_t triangle_index, uint32_t instance_index) {
    return triangle_index << 24 | instance_index;
}

//vert

layout(location = 0) flat out uint32_t packed;

void visbuffer_opaque_vertex() {
    uint32_t triangle_index = gl_VertexIndex / 3;

    MeshletIndex meshlet_index = MeshletIndexBuffer(uniforms.instance_meshlets).meshlet_index[gl_InstanceIndex];
    Instance instance = instances[meshlet_index.instance_index];
    MeshInfo mesh_info = MeshInfoBuffer(instance.mesh_info_address).mesh_info;
    Meshlet meshlet = MeshletBuffer(mesh_info.meshlets).meshlets[meshlet_index.meshlet_index];

    uint32_t micro_index = meshlet.index_offset + MicroIndexBufferSingle(mesh_info.micro_indices + meshlet.triangle_offset).indices[gl_VertexIndex];

    uint32_t index = load_index(mesh_info, micro_index);

    float3 position = calculate_world_pos(instance, mesh_info, index);

    gl_Position = uniforms.combined_perspective_view * float4(position, 1.0);
    packed = pack(triangle_index, gl_InstanceIndex);
}

//frag

layout(location = 0) flat in uint32_t packed;
layout(location = 0) out uint32_t out_packed;

void visbuffer_opaque_pixel() {
    out_packed = packed;
}

//vert

layout(location = 0) flat out uint32_t packed;
layout(location = 1) out float2 uv;
layout(location = 2) flat out uint32_t base_texture_index;
layout(push_constant) uniform pc {ShadowPassConstant shadow_constant;};

void visbuffer_alpha_clip_vertex() {
    uint32_t triangle_index = gl_VertexIndex / 3;

    MeshletIndex meshlet_index = MeshletIndexBuffer(uniforms.instance_meshlets).meshlet_index[gl_InstanceIndex];
    Instance instance = instances[meshlet_index.instance_index];
    MeshInfo mesh_info = MeshInfoBuffer(instance.mesh_info_address).mesh_info;
    Meshlet meshlet = MeshletBuffer(mesh_info.meshlets).meshlets[meshlet_index.meshlet_index];

    uint32_t micro_index = meshlet.index_offset + MicroIndexBufferSingle(mesh_info.micro_indices + meshlet.triangle_offset).indices[gl_VertexIndex];

    uint32_t index = load_index(mesh_info, micro_index);

    float3 position = calculate_world_pos(instance, mesh_info, index);

    gl_Position = uniforms.combined_perspective_view * float4(position, 1.0);
    packed = pack(triangle_index, gl_InstanceIndex);
    base_texture_index = mesh_info.albedo_texture_index;
    uv = float2(QuanitizedUvs(mesh_info.uvs).uvs[index]) * mesh_info.texture_scale + mesh_info.texture_offset;
}

//frag

layout(location = 0) flat in uint32_t packed;
layout(location = 1) in float2 uv;
layout(location = 2) flat in uint32_t base_texture_index;
layout(location = 0) out uint32_t out_packed;

void visbuffer_alpha_clip_pixel() {
    if (texture(sampler2D(textures[base_texture_index], repeat_sampler), uv).a < 0.5) {
        discard;
    }

    out_packed = packed;
}

//vert

layout(push_constant) uniform pc {ShadowPassConstant shadow_constant;};

void shadowmap_opaque_vertex() {
    MeshletIndex meshlet_index = MeshletIndexBuffer(uniforms.instance_meshlets).meshlet_index[gl_InstanceIndex];
    Instance instance = instances[meshlet_index.instance_index];
    MeshInfo mesh_info = MeshInfoBuffer(instance.mesh_info_address).mesh_info;
    Meshlet meshlet = MeshletBuffer(mesh_info.meshlets).meshlets[meshlet_index.meshlet_index];

    uint32_t micro_index = meshlet.index_offset + MicroIndexBufferSingle(mesh_info.micro_indices + meshlet.triangle_offset).indices[gl_VertexIndex];

    uint32_t index = load_index(mesh_info, micro_index);

    float3 position = calculate_world_pos(instance, mesh_info, index);

    gl_Position = misc_storage.shadow_matrices[shadow_constant.cascade_index] * float4(position, 1.0);
}

//vert

layout(location = 0) out float2 uv;
layout(location = 1) flat out uint32_t base_texture_index;
layout(push_constant) uniform pc {ShadowPassConstant shadow_constant;};

void shadowmap_alpha_clip_vertex() {
    MeshletIndex meshlet_index = MeshletIndexBuffer(uniforms.instance_meshlets).meshlet_index[gl_InstanceIndex];
    Instance instance = instances[meshlet_index.instance_index];
    MeshInfo mesh_info = MeshInfoBuffer(instance.mesh_info_address).mesh_info;
    Meshlet meshlet = MeshletBuffer(mesh_info.meshlets).meshlets[meshlet_index.meshlet_index];

    uint32_t micro_index = meshlet.index_offset + MicroIndexBufferSingle(mesh_info.micro_indices + meshlet.triangle_offset).indices[gl_VertexIndex];

    uint32_t index = load_index(mesh_info, micro_index);

    float3 position = calculate_world_pos(instance, mesh_info, index);
    gl_Position = misc_storage.shadow_matrices[shadow_constant.cascade_index] * float4(position, 1.0);

    base_texture_index = mesh_info.albedo_texture_index;
    uv = float2(QuanitizedUvs(mesh_info.uvs).uvs[index]) * mesh_info.texture_scale + mesh_info.texture_offset;
}

//frag

layout(location = 0) in float2 uv;
layout(location = 1) flat in uint32_t base_texture_index;

void shadowmap_alpha_clipped_pixel() {
    if (texture(sampler2D(textures[base_texture_index], repeat_sampler), uv).a < 0.5) {
        discard;
    }
}
