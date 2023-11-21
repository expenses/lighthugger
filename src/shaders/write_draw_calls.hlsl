#include "bindings.hlsl"

[shader("compute")]
[numthreads(64, 1, 1)]
void write_draw_calls(uint3 global_id: SV_DispatchThreadID) {
    uint32_t id = global_id.x;

    if (id >= uniforms.num_instances) {
        return;
    }

    Instance instance = instances[id];

    // cull here

    uint32_t current_draw;
    InterlockedAdd(draw_counts[0], 1, current_draw);

    // Todo: merge draw calls that use the same mesh.

    draw_calls[current_draw].vertexCount = instance.mesh_info.num_indices;
    draw_calls[current_draw].instanceCount = 1;
    draw_calls[current_draw].firstVertex = 0;
    draw_calls[current_draw].firstInstance = id;
}
