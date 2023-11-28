#include "common/bindings.hlsl"
#include "common/loading.hlsl"
#include "common/geometry.hlsl"

// todo: cull on vertical and horizontal planes.
bool cull_bounding_sphere(Instance instance, float3 position, float radius) {
    float3 scale = float3(
        length(instance.transform[0].xyz),
        length(instance.transform[1].xyz),
        length(instance.transform[2].xyz)
    );
    // 99% of the time scales will be uniform. But in the chance they're not,
    // use the longest dimension.
    float scale_scalar = max(max(scale.x, scale.y), scale.z);

    radius *= scale_scalar;

    float3 view_space_pos = mul(uniforms.initial_view, float4(position, 1.0)).xyz;
    // Flip z, not 100% sure why, copied this from older code.
    view_space_pos.z = -view_space_pos.z;

    return view_space_pos.z + radius <= NEAR_PLANE;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void write_draw_calls(uint3 global_id: SV_DispatchThreadID) {
    uint32_t id = global_id.x;

    if (id >= misc_storage[0].num_expanded_meshlets) {
        return;
    }

    MeshletIndex meshlet_index = load_instance_meshlet(uniforms.expanded_meshlets, id);

    Instance instance = load_instance(meshlet_index.instance_index);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    Meshlet meshlet = load_meshlet(mesh_info.meshlets, meshlet_index.meshlet_index);

    if (cull_bounding_sphere(instance, mul(instance.transform, float4(meshlet.center, 1.0)).xyz, meshlet.radius)) {
        return;
    }

    uint32_t draw_call = 0;
    uint meshlet_offset;

    if (mesh_info.flags & MESH_INFO_FLAGS_ALPHA_CLIP) {
        InterlockedAdd(misc_storage[0].num_alpha_clip_meshlets, 1, meshlet_offset);
        meshlet_offset += ALPHA_CLIP_DRAWS_OFFSET;
        draw_call = 1;
    } else {
        InterlockedAdd(misc_storage[0].num_opaque_meshlets, 1, meshlet_offset);
    }

    InterlockedAdd(draw_calls[draw_call].vertexCount, MAX_MESHLET_VERTICES);
    draw_calls[0].instanceCount = 1;
    draw_calls[0].firstVertex = 0;
    draw_calls[0].firstInstance = 0;

    draw_calls[1].instanceCount = 1;
    draw_calls[1].firstVertex = 0;
    draw_calls[1].firstInstance = 0;

    uint64_t write_address = uniforms.instance_meshlets + (meshlet_offset) * sizeof(MeshletIndex);
    vk::RawBufferStore<uint32_t>(write_address, meshlet_index.instance_index);
    write_address += sizeof(uint32_t);
    vk::RawBufferStore<uint32_t>(write_address, meshlet_index.meshlet_index);
}

[shader("compute")]
[numthreads(64, 1, 1)]
void expand_meshlets(uint3 global_id: SV_DispatchThreadID) {
    uint32_t id = global_id.x;

    if (id >= uniforms.num_instances) {
        return;
    }

    Instance instance = load_instance(id);
    MeshInfo mesh_info = load_mesh_info(instance.mesh_info_address);

    // Extract position and scale from transform matrix
    float3 position = float3(instance.transform[0][3], instance.transform[1][3], instance.transform[2][3]);

    if (cull_bounding_sphere(instance, position, mesh_info.bounding_sphere_radius)) {
        return;
    }

    uint32_t meshlets_offset;
    InterlockedAdd(misc_storage[0].num_expanded_meshlets, mesh_info.num_meshlets, meshlets_offset);

    for (uint32_t i = 0; i < mesh_info.num_meshlets; i++) {
        uint64_t write_address = uniforms.expanded_meshlets + (meshlets_offset + i) * sizeof(MeshletIndex);
        vk::RawBufferStore<uint32_t>(write_address, id);
        write_address += sizeof(uint32_t);
        vk::RawBufferStore<uint32_t>(write_address, i);
    }
}
