#include "common/bindings.hlsl"
#include "common/loading.hlsl"
#include "common/geometry.hlsl"

[shader("compute")]
[numthreads(64, 1, 1)]
void write_draw_calls(uint3 global_id: SV_DispatchThreadID) {
    uint32_t id = global_id.x;

    if (id >= uniforms.num_instances) {
        return;
    }

    Instance instance = load_instance(id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    // Extract position and scale from transform matrix
    float3 position = float3(instance.transform[0][3], instance.transform[1][3], instance.transform[2][3]);
    // todo: not 100% sure if this is correct. Might need to be transposed.
    float3 scale = float3(
        length(instance.transform[0].xyz),
        length(instance.transform[1].xyz),
        length(instance.transform[2].xyz)
    );
    // 99% of the time scales will be uniform. But in the chance they're not,
    // use the longest dimension.
    float scale_scalar = max(max(scale.x, scale.y), scale.z);

    float bounding_sphere_radius = mesh_info.bounding_sphere_radius * scale_scalar;

    float3 sphere_center = mul(uniforms.view, float4(position, 1.0)).xyz;
    // Flip z, not 100% sure why, copied this from older code.
    sphere_center.z = -sphere_center.z;

    // Cull any objects completely behind the camera.
    if (sphere_center.z + bounding_sphere_radius <= NEAR_PLANE) {
        return;
    }

    // todo: cull on vertical and horizontal planes.

    uint32_t meshlets_offset;
    uint32_t draw_call = 0;

    if (mesh_info.flags & MESH_INFO_FLAGS_ALPHA_CLIP) {
        InterlockedAdd(misc_storage[0].num_opaque_meshlets, mesh_info.num_meshlets, meshlets_offset);
        meshlets_offset += ALPHA_CLIP_DRAWS_OFFSET;
        draw_call = 1;
    } else {
        InterlockedAdd(misc_storage[0].num_alpha_clip_meshlets, mesh_info.num_meshlets, meshlets_offset);
    }

    InterlockedAdd(draw_calls[draw_call].vertexCount, MAX_MESHLET_VERTICES * mesh_info.num_meshlets);
    draw_calls[0].instanceCount = 1;
    draw_calls[0].firstVertex = 0;
    draw_calls[0].firstInstance = 0;

    draw_calls[1].instanceCount = 1;
    draw_calls[1].firstVertex = 0;
    draw_calls[1].firstInstance = 0;

    for (uint32_t i = 0; i < mesh_info.num_meshlets; i++) {
        uint64_t write_address = uniforms.instance_meshlets + (meshlets_offset + i) * sizeof(MeshletIndex);
        vk::RawBufferStore<uint32_t>(write_address, id);
        write_address += sizeof(uint32_t);
        vk::RawBufferStore<uint32_t>(write_address, i);
    }
}
