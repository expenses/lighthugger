#include "common/bindings.hlsl"
#include "common/loading.hlsl"
#include "common/geometry.hlsl"

// todo: cull on vertical and horizontal planes.
bool cull_bounding_sphere(Instance instance, float4 bounding_sphere) {
    float3 world_space_pos = mul(instance.transform, float4(bounding_sphere.xyz, 1.0)).xyz;
    float radius = bounding_sphere.w;

    float3 scale = float3(
        length(instance.transform[0].xyz),
        length(instance.transform[1].xyz),
        length(instance.transform[2].xyz)
    );
    // 99% of the time scales will be uniform. But in the chance they're not,
    // use the longest dimension.
    float scale_scalar = max(max(scale.x, scale.y), scale.z);

    radius *= scale_scalar;

    float3 view_space_pos = mul(uniforms.initial_view, float4(world_space_pos, 1.0)).xyz;
    // The view space goes from negatives in the front to positives in the back.
    // This is confusing so flipping it here makes sense I think.
    view_space_pos.z = -view_space_pos.z;

    // Is the most positive/forwards point of the object in front of the near plane?
    bool visible = view_space_pos.z + radius > NEAR_PLANE;

    // Do some fancy stuff by getting the frustum planes and comparing the position against them.
    float3 frustum_x = normalize(uniforms.perspective[3].xyz + uniforms.perspective[0].xyz);
    float3 frustum_y = normalize(uniforms.perspective[3].xyz + uniforms.perspective[1].xyz);

    visible &= view_space_pos.z * frustum_x.z + abs(view_space_pos.x) * frustum_x.x < radius;
    visible &= view_space_pos.z * frustum_y.z - abs(view_space_pos.y) * frustum_y.y < radius;

    return !visible;
}

bool cull_cone_perspective(Instance instance, Meshlet meshlet) {
    float3 apex = mul(instance.transform, float4(meshlet.cone_apex, 1.0)).xyz;
    float3 axis = normalize(mul(instance.normal_transform, meshlet.cone_axis));

    return dot(normalize(apex - uniforms.camera_pos), normalize(axis)) >= meshlet.cone_cutoff;
}

bool cull_cone_orthographic(Instance instance, Meshlet meshlet) {
    float3 axis = normalize(mul(instance.normal_transform, meshlet.cone_axis));
    return dot(uniforms.sun_dir, axis) >= meshlet.cone_cutoff;
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

    if (cull_bounding_sphere(instance, meshlet.bounding_sphere)) {
        return;
    }

    if (cull_cone_perspective(instance, meshlet)) {
        return;
    }

    uint meshlet_offset;

    if (mesh_info.flags & MESH_INFO_FLAGS_ALPHA_CLIP) {
        InterlockedAdd(misc_storage[0].num_alpha_clip_meshlets, 1, meshlet_offset);
        meshlet_offset += ALPHA_CLIP_DRAWS_OFFSET;
    } else {
        InterlockedAdd(misc_storage[0].num_opaque_meshlets, 1, meshlet_offset);
    }


    draw_calls[meshlet_offset].vertexCount = meshlet.triangle_count * 3;
    draw_calls[meshlet_offset].instanceCount = 1;
    draw_calls[meshlet_offset].firstVertex = 0;
    draw_calls[meshlet_offset].firstInstance = meshlet_offset;

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

    if (cull_bounding_sphere(instance, mesh_info.bounding_sphere)) {
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
