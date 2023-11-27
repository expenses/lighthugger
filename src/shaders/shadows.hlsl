#include "common/geometry.hlsl"

[[vk::push_constant]]
ShadowPassConstant shadow_constant;

[shader("vertex")]
float4 vertex(
    uint32_t vertex_index : SV_VertexID, uint32_t instance_index: SV_InstanceID
): SV_Position
{
    MeshletIndex meshlet_index = load_instance_meshlet(uniforms.instance_meshlets, instance_index);

    Instance instance = load_instance(meshlet_index.instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);
    Meshlet meshlet = load_meshlet(mesh_info.meshlets, meshlet_index.meshlet_index);
    uint16_t micro_index = load_uint8_t(mesh_info.micro_indices + meshlet.triangle_offset + vertex_index);

    uint32_t index = load_index(mesh_info, meshlet.index_offset + micro_index);

    return calculate_view_pos_position(
        instance,
        mesh_info,
        index,
        misc_storage[0].shadow_matrices[shadow_constant.cascade_index]
    );
}

struct VaryingsAlphaClip {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] float2 uv: ATTRIB0;
    [[vk::location(2)]] uint32_t albedo_texture_index: ATTRIB1;
};

[shader("vertex")]
VaryingsAlphaClip vertex_alpha_clip(
    uint32_t vertex_index : SV_VertexID, uint32_t instance_index: SV_InstanceID
)
{
    MeshletIndex meshlet_index = load_instance_meshlet(uniforms.instance_meshlets, instance_index);

    Instance instance = load_instance(meshlet_index.instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);
    Meshlet meshlet = load_meshlet(mesh_info.meshlets, meshlet_index.meshlet_index);
    uint16_t micro_index = load_uint8_t(mesh_info.micro_indices + meshlet.triangle_offset + vertex_index);

    uint32_t index = load_index(mesh_info, meshlet.index_offset + micro_index);

    VaryingsAlphaClip varyings;
    varyings.clip_pos = calculate_view_pos_position(
        instance,
        mesh_info,
        index,
        misc_storage[0].shadow_matrices[shadow_constant.cascade_index]
    );
    varyings.albedo_texture_index = mesh_info.albedo_texture_index;
    varyings.uv = float2(load_value<uint16_t2>(mesh_info.uvs, index)) * mesh_info.texture_scale + mesh_info.texture_offset;
    return varyings;
}

[shader("pixel")]
void pixel_alpha_clip(
    VaryingsAlphaClip input
) {
    if (textures[input.albedo_texture_index].Sample(repeat_sampler, input.uv).a < 0.5) {
        discard;
    }
}
