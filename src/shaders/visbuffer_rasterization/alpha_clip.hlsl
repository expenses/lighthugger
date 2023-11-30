#include "../common/geometry.hlsl"

struct Varyings {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] uint32_t packed: ATTRIB0;
    [[vk::location(2)]] float2 uv: ATTRIB1;
    [[vk::location(3)]] uint32_t albedo_texture_index: ATTRIB2;
};

[shader("vertex")]
Varyings vertex(
    uint32_t global_vertex_index : SV_VertexID
) {
    uint32_t instance_index = global_vertex_index / MAX_MESHLET_VERTICES;
    uint32_t vertex_index = global_vertex_index % MAX_MESHLET_VERTICES;
    uint32_t triangle_index = vertex_index / 3;
    instance_index += ALPHA_CLIP_DRAWS_OFFSET;

    MeshletIndex meshlet_index = load_instance_meshlet(uniforms.instance_meshlets, instance_index);
    Instance instance = load_instance(meshlet_index.instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);
    Meshlet meshlet = load_meshlet(mesh_info.meshlets, meshlet_index.meshlet_index);

    // Cull extra triangles bu setting all indices to 0.
    vertex_index = select(triangle_index < meshlet.triangle_count, vertex_index, 0);

    uint16_t micro_index = load_uint8_t(mesh_info.micro_indices + meshlet.triangle_offset + vertex_index);

    uint32_t index = load_index(mesh_info, meshlet.index_offset + micro_index);

    Varyings varyings;
    varyings.clip_pos =  calculate_view_pos_position(instance, mesh_info, index);
    varyings.packed = pack(triangle_index, instance_index);
    varyings.albedo_texture_index = mesh_info.albedo_texture_index;
    varyings.uv = float2(load_value<uint16_t2>(mesh_info.uvs, index)) * mesh_info.texture_scale + mesh_info.texture_offset;
    return varyings;
}

[shader("pixel")]
void pixel(
    Varyings input,
    [[vk::location(0)]] out uint32_t packed: SV_Target0
) {
    if (textures[input.albedo_texture_index].Sample(repeat_sampler, input.uv).a < 0.5) {
        discard;
    }

    packed = input.packed;
}
