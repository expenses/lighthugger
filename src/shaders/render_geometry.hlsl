#include "common/bindings.hlsl"
#include "common/debug.hlsl"
#include "common/loading.hlsl"

[shader("vertex")]
float4 depth_only(
    uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID
): SV_Position
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    float3 position;

    uint32_t offset = load_value<uint32_t>(mesh_info.indices, vertex_id);

    if (mesh_info.type == TYPE_QUANITZED) {
        position = float3(load_uint16_t3(mesh_info.positions, offset));
    } else {
        position = load_value<float3>(mesh_info.positions, offset);
    }

    float3 world_pos = mul(instance.transform, float4(position, 1.0)).xyz;

    return mul(uniforms.combined_perspective_view, float4(world_pos, 1.0));
}

[[vk::push_constant]]
ShadowPassConstant shadow_constant;

[shader("vertex")]
float4 shadow_pass(
    uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID
): SV_Position
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    float3 position;

    uint32_t offset = load_value<uint32_t>(mesh_info.indices, vertex_id);

    if (mesh_info.type == TYPE_QUANITZED) {
        position = float3(load_uint16_t3(mesh_info.positions, offset));
    } else {
        position = load_value<float3>(mesh_info.positions, offset);
    }

    float3 world_pos = mul(instance.transform, float4(position, 1.0)).xyz;

    return mul(depth_info[0].shadow_rendering_matrices[shadow_constant.cascade_index], float4(world_pos, 1.0));
}

struct Varyings {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] float3 world_pos : ATTRIB0;
    [[vk::location(2)]] float3 normal: ATTRIB1;
    [[vk::location(3)]] float2 uv: ATTRIB2;
    [[vk::location(4)]] uint32_t material_index: ATTRIB3;
    [[vk::location(5)]] uint32_t instance_index: ATTRIB4;
};

[shader("vertex")]
Varyings VSMain(uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID)
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    Varyings varyings;
    varyings.instance_index = instance_id;

    uint32_t offset = load_value<uint32_t>(mesh_info.indices, vertex_id);

    float3 position;

    if (mesh_info.type == TYPE_QUANITZED) {
        varyings.material_index = uint32_t(mesh_info.material_indices);

        position = float3(load_uint16_t3(mesh_info.positions, offset));
        MaterialInfo material_info = load_material_info(mesh_info.material_info, varyings.material_index);
        varyings.uv = float2(load_value<uint16_t2>(mesh_info.uvs, offset)) * material_info.albedo_texture_scale + material_info.albedo_texture_offset;

        // Load 3 x i8 with a padding byte.
        int16_t4 normal_unpacked = unpack_s8s16(load_value<int8_t4_packed>(mesh_info.normals, offset));
        varyings.normal = normalize(float3(normal_unpacked.xyz));
    } else {
        position = load_value<float3>(mesh_info.positions, offset);
        varyings.normal = load_value<float3>(mesh_info.normals, offset);
        varyings.uv = load_value<float2>(mesh_info.uvs, offset);
        varyings.material_index = load_value<uint16_t>(mesh_info.material_indices, offset);
    }

    varyings.world_pos = mul(instance.transform, float4(position, 1.0)).xyz;
    varyings.normal = mul(instance.normal_transform, varyings.normal);
    varyings.clip_pos = mul(uniforms.combined_perspective_view, float4(varyings.world_pos, 1.0));

    return varyings;
}

static const float4x4 bias_matrix = float4x4(
    0.5, 0.0, 0.0, 0.5,
    0.0, 0.5, 0.0, 0.5,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0
);

[shader("pixel")]
void PSMain(
    Varyings input,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    Instance instance = load_instance(input.instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    MaterialInfo material_info = load_material_info(mesh_info.material_info, input.material_index);
    uint32_t cascade_index;
    float4 shadow_coord = float4(0,0,0,0);
    for (cascade_index = 0; cascade_index < 4; cascade_index++) {
        // Get the coordinate in shadow view space.
        shadow_coord = mul(depth_info[0].shadow_rendering_matrices[cascade_index], float4(input.world_pos, 1.0));
        // If it's inside the cascade view space (which is NDC so -1 to 1) then stop as
        // this is the highest quality cascade for the fragment.
        if (abs(shadow_coord.x) < 1.0 && abs(shadow_coord.y) < 1.0) {
            break;
        }
    }

    float4 shadow_view_coord = mul(bias_matrix, shadow_coord);
    shadow_view_coord /= shadow_view_coord.w;
    float shadow_sum = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) / 1024.0;
            shadow_sum += shadowmap.SampleCmpLevelZero(
                shadowmap_comparison_sampler,
                float3(shadow_view_coord.xy + offset, cascade_index),
                shadow_view_coord.z
            );
        }
    }
    shadow_sum /= 9.0;

    float3 normal = normalize(input.normal);

    float n_dot_l = max(dot(uniforms.sun_dir, normal), 0.0);
    float3 albedo = textures[material_info.albedo_texture_index].Sample(repeat_sampler, input.uv).rgb;

    float ambient = 0.05;

    float3 lighting = max(n_dot_l * shadow_sum * uniforms.sun_intensity, ambient);

    float3 diffuse = albedo * lighting;

    target_0 = float4(diffuse, 1.0);

    if (uniforms.debug_cascades) {
        float3 debug_col = DEBUG_COLOURS[cascade_index];
        target_0 = float4(albedo * debug_col, 1.0);
    }
}
