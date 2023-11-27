#pragma once

#include "bindings.hlsl"

Instance load_instance(uint32_t offset) {
    return instances.Load<Instance>(offset * sizeof(Instance));
}

uint32_t load_index(MeshInfo mesh_info, uint32_t vertex_id) {
    if (mesh_info.flags & MESH_INFO_FLAGS_32_BIT_INDICES) {
        return load_value<uint32_t>(mesh_info.indices, vertex_id);
    } else {
        return load_value<uint16_t>(mesh_info.indices, vertex_id);
    }
}

float3 calculate_world_pos(
    Instance instance,
    MeshInfo mesh_info,
    uint32_t index
) {
    float3 position = float3(load_uint16_t3(mesh_info.positions, index));
    return mul(instance.transform, float4(position, 1.0)).xyz;
}

float4 calculate_view_pos_position(
    Instance instance,
    MeshInfo mesh_info,
    uint32_t index,
    float4x4 perspective_view_matrix
) {
    float3 world_pos = calculate_world_pos(instance, mesh_info, index);
    return mul(perspective_view_matrix, float4(world_pos, 1.0));
}


float4 calculate_view_pos_position(
    Instance instance,
    MeshInfo mesh_info,
    uint32_t index
) {
    return calculate_view_pos_position(instance, mesh_info, index, uniforms.combined_perspective_view);
}
