#include "bindings.hlsl"

[shader("compute")]
[numthreads(64, 1, 1)]
void write_draw_calls(uint3 global_id: SV_DispatchThreadID) {
    uint id = global_id.x;
    uint num_items;
    uint stride;
    mesh_buffer_addresses.GetDimensions(num_items, stride);

    if (id >= num_items) {
        return;
    }

    MeshBufferAddresses addresses = mesh_buffer_addresses[id];

    draw_calls[id].vertexCount = addresses.num_indices;
    draw_calls[id].instanceCount = 1;
    draw_calls[id].firstVertex = 0;
    draw_calls[id].firstInstance = 0;
}
