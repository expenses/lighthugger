#include "common/loading.hlsl"

[[vk::binding(0)]] RWByteAddressBuffer mesh_info_b;

MeshInfo load_mesh_info(RWByteAddressBuffer byte_address_buffer) {
    MeshInfo info;
    uint32_t offset = 0;
    //info.positions = 0;
    info.positions = load_value<uint64_t>(byte_address_buffer, offset);
    //info.indices = load_value_offset_zero<uint64_t>(address);
    //info.normals = load_value_offset_zero<uint64_t>(address);
    //info.uvs = load_value_offset_zero<uint64_t>(address);
    //info.material_indices = load_value_offset_zero<uint64_t>(address);
    //info.material_info = load_value_offset_zero<uint64_t>(address);
    //info.num_indices = load_value_offset_zero<uint32_t>(address);
    //info.type = load_value_offset_zero<uint32_t>(address);
    //info.bounding_sphere_radius = load_value_offset_zero<float>(address);
    return info;
}


[shader("compute")]
[numthreads(64, 1, 1)]
void write_draw_calls(uint3 global_id: SV_DispatchThreadID) {
    MeshInfo mesh_info = load_mesh_info(mesh_info_b);//.mesh_info_address);

    float3 position;

    if (mesh_info.type == 1) {
        position = float3(load_value<uint16_t4>(mesh_info.positions, global_id.x).xyz);
    } else {
        position = load_value<float3>(mesh_info.positions, global_id.x);
    }

    uint32_t side_length = asuint(length(position));

    uint32_t subgroup_max = WaveActiveMax(side_length);

    uint32_t offset = sizeof(uint64_t) * 6 + sizeof(uint32_t) * 2;

    if (WaveIsFirstLane()) {
        mesh_info_b.InterlockedMax(offset, subgroup_max);
    }
}
