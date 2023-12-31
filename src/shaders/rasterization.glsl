#include "common/bindings.glsl"
#include "common/util.glsl"

//vert

struct VertexData {
    Instance instance;
    MeshInfo mesh_info;
    uint32_t index;
};

VertexData load_vertex_data(uint32_t vertex_index, uint32_t instance_index) {
    VertexData vertex_data;

    MeshletReference meshlet_reference =
        MeshletReferenceBuffer(get_uniforms().meshlet_references)
            .meshlet_reference[instance_index];
    vertex_data.instance = InstanceBuffer(get_uniforms().instances)
                               .instances[meshlet_reference.instance_index];
    vertex_data.mesh_info =
        MeshInfoBuffer(vertex_data.instance.mesh_info_address).mesh_info;
    Meshlet meshlet = MeshletBuffer(vertex_data.mesh_info.meshlets)
                          .meshlets[meshlet_reference.meshlet_index];

    uint32_t micro_index = meshlet.index_offset
        + MicroIndexBufferSingle(
              vertex_data.mesh_info.micro_indices + meshlet.triangle_offset
        )
              .indices[vertex_index];

    vertex_data.index = load_index(vertex_data.mesh_info, micro_index);

    return vertex_data;
}

layout(location = 0) flat out uint32_t packed;

void visbuffer_opaque_vertex() {
    uint32_t triangle_index = gl_VertexIndex / 3;

    VertexData data = load_vertex_data(gl_VertexIndex, gl_InstanceIndex);

    float3 position =
        calculate_world_pos(data.instance, data.mesh_info, data.index);

    gl_Position =
        get_uniforms().combined_perspective_view * float4(position, 1.0);
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

void visbuffer_alpha_clip_vertex() {
    uint32_t triangle_index = gl_VertexIndex / 3;

    VertexData data = load_vertex_data(gl_VertexIndex, gl_InstanceIndex);

    float3 position =
        calculate_world_pos(data.instance, data.mesh_info, data.index);

    gl_Position =
        get_uniforms().combined_perspective_view * float4(position, 1.0);
    packed = pack(triangle_index, gl_InstanceIndex);
    base_texture_index = data.mesh_info.base_color_texture_index;
    uv = float2(QuanitizedUvs(data.mesh_info.uvs).uvs[data.index])
            * data.mesh_info.texture_scale
        + data.mesh_info.texture_offset;
}

//frag

layout(location = 0) flat in uint32_t packed;
layout(location = 1) in float2 uv;
layout(location = 2) flat in uint32_t base_texture_index;
layout(location = 0) out uint32_t out_packed;

void visbuffer_alpha_clip_pixel() {
    if (texture(sampler2D(textures[base_texture_index], repeat_sampler), uv).a
        < 0.5) {
        discard;
    }

    out_packed = packed;
}

//vert

void shadowmap_opaque_vertex() {
    VertexData data = load_vertex_data(gl_VertexIndex, gl_InstanceIndex);

    float3 position =
        calculate_world_pos(data.instance, data.mesh_info, data.index);

    gl_Position =
        MiscStorageBuffer(get_uniforms().misc_storage)
            .misc_storage.shadow_matrices[shadow_constant.cascade_index]
        * float4(position, 1.0);
}

//vert

layout(location = 0) out float2 uv;
layout(location = 1) flat out uint32_t base_texture_index;

void shadowmap_alpha_clip_vertex() {
    VertexData data = load_vertex_data(gl_VertexIndex, gl_InstanceIndex);

    float3 position =
        calculate_world_pos(data.instance, data.mesh_info, data.index);

    gl_Position =
        MiscStorageBuffer(get_uniforms().misc_storage)
            .misc_storage.shadow_matrices[shadow_constant.cascade_index]
        * float4(position, 1.0);

    base_texture_index = data.mesh_info.base_color_texture_index;
    uv = float2(QuanitizedUvs(data.mesh_info.uvs).uvs[data.index])
            * data.mesh_info.texture_scale
        + data.mesh_info.texture_offset;
}

//frag

layout(location = 0) in float2 uv;
layout(location = 1) flat in uint32_t base_texture_index;

void shadowmap_alpha_clipped_pixel() {
    if (texture(sampler2D(textures[base_texture_index], repeat_sampler), uv).a
        < 0.5) {
        discard;
    }
}
