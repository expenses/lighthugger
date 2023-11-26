#include "../common/geometry.hlsl"

struct Varyings {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] uint32_t packed: ATTRIB0;
    [[vk::location(2)]] float2 uv: ATTRIB1;
    [[vk::location(3)]] uint32_t albedo_texture_index: ATTRIB2;
};

[shader("vertex")]
Varyings vertex(
    uint32_t vertex_index : SV_VertexID, uint32_t instance_index: SV_InstanceID
) {
    Instance instance = load_instance(instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);
    Varyings varyings;
    uint32_t index = load_index(mesh_info, vertex_index);
    varyings.clip_pos =  calculate_view_pos_position(instance, mesh_info, index);
    varyings.packed = (vertex_index / 3) << 16 | instance_index;
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
