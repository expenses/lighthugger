#include "common/bindings.hlsl"
#include "common/debug.hlsl"
#include "common/loading.hlsl"
#include "common/vbuffer.hlsl"

uint32_t load_index(MeshInfo mesh_info, uint32_t vertex_id) {
    if (mesh_info.flags & MESH_INFO_FLAGS_32_BIT_INDICES) {
        return load_value<uint32_t>(mesh_info.indices, vertex_id);
    } else {
        return load_value<uint16_t>(mesh_info.indices, vertex_id);
    }
}

float4 calculate_view_pos_position(
    Instance instance,
    MeshInfo mesh_info,
    uint32_t index,
    float4x4 perspective_view_matrix
) {
    float3 position = float3(load_uint16_t3(mesh_info.positions, index));
    float3 world_pos = mul(instance.transform, float4(position, 1.0)).xyz;
    return mul(perspective_view_matrix, float4(world_pos, 1.0));
}


float4 calculate_view_pos_position(
    Instance instance,
    MeshInfo mesh_info,
    uint32_t index
) {
    return calculate_view_pos_position(instance, mesh_info, index, uniforms.combined_perspective_view);
}

[shader("vertex")]
float4 depth_only(
    uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID
): SV_Position
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);
    return calculate_view_pos_position(instance, mesh_info, load_index(mesh_info, vertex_id));
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

    return calculate_view_pos_position(
        instance,
        mesh_info,
        load_index(mesh_info, vertex_id),
        misc_storage[0].shadow_rendering_matrices[shadow_constant.cascade_index]
    );
}

struct Varyings {
    [[vk::location(0)]] float4 clip_pos : SV_Position;
    [[vk::location(1)]] float3 world_pos : ATTRIB0;
    [[vk::location(2)]] float3 normal: ATTRIB1;
    [[vk::location(3)]] float2 uv: ATTRIB2;
    [[vk::location(4)]] uint32_t instance_index: ATTRIB3;
    //[[vk::location(5)]] float2 barycentrics: ATTRIB4;
};

[shader("vertex")]
Varyings VSMain(uint32_t vertex_id : SV_VertexID, uint32_t instance_id: SV_InstanceID)
{
    Instance instance = load_instance(instance_id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    Varyings varyings;
    varyings.instance_index = instance_id;

    //varyings.barycentrics = float2(vertex_id % 3 == 0, vertex_id % 3 == 1);

    uint32_t offset;

    if (mesh_info.flags & MESH_INFO_FLAGS_32_BIT_INDICES) {
        offset = load_value<uint32_t>(mesh_info.indices, vertex_id);
    } else {
        offset = load_value<uint16_t>(mesh_info.indices, vertex_id);
    }
    float3 position = float3(load_uint16_t3(mesh_info.positions, offset));
    varyings.uv = float2(load_value<uint16_t2>(mesh_info.uvs, offset));
    varyings.uv = varyings.uv * mesh_info.texture_scale + mesh_info.texture_offset;

    // Load 3 x i8 with a padding byte.
    int16_t4 normal_unpacked = unpack_s8s16(load_value<int8_t4_packed>(mesh_info.normals, offset));
    varyings.normal = normalize(float3(normal_unpacked.xyz));

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

struct Material {
    float3 albedo;
    float metallic;
    float roughness;
};

[shader("pixel")]
void PSMain(
    Varyings input,
    [[vk::location(0)]] out float4 target_0: SV_Target0
) {
    Instance instance = load_instance(input.instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    uint32_t cascade_index;
    float4 shadow_coord = float4(0,0,0,0);
    for (cascade_index = 0; cascade_index < 4; cascade_index++) {
        // Get the coordinate in shadow view space.
        shadow_coord = mul(misc_storage[0].shadow_rendering_matrices[cascade_index], float4(input.world_pos, 1.0));
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

    if (shadow_view_coord.z > 1) {
        shadow_sum = 1.0;
    }

    Material material;

    float3 normal = normalize(input.normal);

    float n_dot_l = max(dot(uniforms.sun_dir, normal), 0.0);
    material.albedo = textures[mesh_info.albedo_texture_index].Sample(repeat_sampler, input.uv).rgb;
    float2 metallic_roughness = textures[mesh_info.metallic_roughness_texture_index].Sample(repeat_sampler, input.uv).yx;
    material.roughness = metallic_roughness.x;
    material.metallic = metallic_roughness.y;

    float ambient = 0.05;

    float3 lighting = max(n_dot_l * shadow_sum * uniforms.sun_intensity, ambient);

    float3 diffuse = material.albedo * lighting;

    target_0 = float4(diffuse, 1.0);

    if (uniforms.debug_cascades) {
        float3 debug_col = DEBUG_COLOURS[cascade_index];
        target_0 = float4(material.albedo * debug_col, 1.0);
    }

    //target_0 = float4(input.barycentrics.x, input.barycentrics.y, 1.0 - input.barycentrics.x - input.barycentrics.y, 1.0);
}
