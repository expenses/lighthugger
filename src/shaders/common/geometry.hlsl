#pragma once

#include "bindings.hlsl"

Instance load_instance(uint32_t offset) {
    uint32_t packed_instance_size = 112;
    offset = offset * packed_instance_size;
    Instance instance;
    instance.transform = load_value_and_increment_offset<float4x4>(instances, offset);
    instance.mesh_info_address = load_value_and_increment_offset<uint64_t>(instances, offset);
    instance.normal_transform = float3x3(
        load_value_and_increment_offset<float3>(instances, offset),
        load_value_and_increment_offset<float3>(instances, offset),
        load_value_and_increment_offset<float3>(instances, offset)
    );
    return instance;
}

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