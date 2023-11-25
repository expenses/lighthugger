#include "common/loading.hlsl"

// Todo: can we use atomic max with buffer device address
// and get rid of this binding?
[[vk::binding(0)]] RWStructuredBuffer<MeshInfoWithUintBoundingSphereRadius> mesh_info_buffer;

[shader("compute")]
[numthreads(64, 1, 1)]
void calc_bounding_sphere(uint3 global_id: SV_DispatchThreadID) {
    uint32_t index = global_id.x;

    MeshInfoWithUintBoundingSphereRadius mesh_info = mesh_info_buffer[0];
    float3 position;

    if (index >= mesh_info.num_vertices) {
        return;
    }

    if (mesh_info.type == TYPE_QUANITZED) {
        position = float3(load_uint16_t3(mesh_info.positions, global_id.x));
    } else {
        position = load_value<float3>(mesh_info.positions, global_id.x);
    }

    uint32_t side_length = asuint(length(position));

    uint32_t subgroup_max = WaveActiveMax(side_length);

    if (WaveIsFirstLane()) {
        InterlockedMax(mesh_info_buffer[0].bounding_sphere_radius, subgroup_max);
    }
}
