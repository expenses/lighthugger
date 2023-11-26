#include "common/geometry.hlsl"

[[vk::push_constant]]
ShadowPassConstant shadow_constant;

[shader("vertex")]
float4 vertex(
    uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID
): SV_Position
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    return calculate_view_pos_position(
        instance,
        mesh_info,
        load_index(mesh_info, vertex_id),
        misc_storage[0].shadow_rendering_matrices[shadow_constant.cascade_index]
    );
}

struct VaryingsAlphaClip {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] float2 uv: ATTRIB0;
    [[vk::location(2)]] uint32_t albedo_texture_index: ATTRIB1;
};

[shader("vertex")]
VaryingsAlphaClip vertex_alpha_clip(
    uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID
)
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    VaryingsAlphaClip varyings;
    uint32_t index = load_index(mesh_info, vertex_id);
    varyings.clip_pos = calculate_view_pos_position(
        instance,
        mesh_info,
        index,
        misc_storage[0].shadow_rendering_matrices[shadow_constant.cascade_index]
    );
    varyings.albedo_texture_index = mesh_info.albedo_texture_index;
    varyings.uv = float2(load_value<uint16_t2>(mesh_info.uvs, index)) * mesh_info.texture_scale + mesh_info.texture_offset;
    return varyings;
}
