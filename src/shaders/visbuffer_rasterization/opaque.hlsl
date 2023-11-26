#include "../common/geometry.hlsl"

struct Varyings {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] uint32_t packed: ATTRIB0;
};

[shader("vertex")]
Varyings vertex(
    uint32_t vertex_index : SV_VertexID, uint32_t instance_index: SV_InstanceID
) {
    Instance instance = load_instance(instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);
    Varyings varyings;
    varyings.clip_pos =  calculate_view_pos_position(instance, mesh_info, load_index(mesh_info, vertex_index));
    varyings.packed = (vertex_index / 3) << 16 | instance_index;
    return varyings;
}

[shader("pixel")]
void pixel(
    Varyings input,
    [[vk::location(0)]] out uint32_t packed: SV_Target0
) {
    packed = input.packed;
}
