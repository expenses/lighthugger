#include "bindings.hlsl"

[shader("compute")]
[numthreads(64, 1, 1)]
void write_draw_calls(uint3 global_id: SV_DispatchThreadID) {
    uint id = global_id.x;

    if (id >= uniforms.num_instances) {
        return;
    }

    Instance instance = instances[id];
    MeshBufferAddresses addresses = mesh_buffer_addresses[instance.mesh_index];

    draw_calls[id].vertexCount = addresses.num_indices;
    draw_calls[id].instanceCount = 1;
    draw_calls[id].firstVertex = 0;
    draw_calls[id].firstInstance = id;
}
