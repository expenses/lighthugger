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

    float z_near = 0.01;

    // Cull any objects completely behind the camera.
    if (sphere_center.z + bounding_sphere_radius <= z_near) {
        return;
    }

    // todo: cull on vertical and horizontal planes.

    uint32_t current_draw;

    if (mesh_info.flags & MESH_INFO_FLAGS_ALPHA_CLIP) {
        InterlockedAdd(misc_storage[0].num_alpha_clip_draws, 1, current_draw);
        current_draw += 512;
    } else {
        InterlockedAdd(misc_storage[0].num_opaque_draws, 1, current_draw);
    }


    // Todo: merge draw calls that use the same mesh.

    draw_calls[current_draw].vertexCount = mesh_info.num_indices;
    draw_calls[current_draw].instanceCount = 1;
    draw_calls[current_draw].firstVertex = 0;
    draw_calls[current_draw].firstInstance = id;
}
