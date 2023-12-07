#include "common/bindings.glsl"

// Prefix sum for 32-bit values using 64-bit atomics.
// Super simple, inspired by Nabla's implementation:
// https://github.com/Devsh-Graphics-Programming/Nabla/blob/8da4b980b5617802ea4e96bf101ddbfe94721a51/include/nbl/builtin/glsl/scanning_append/scanning_append.glsl
//
// See also https://research.nvidia.com/sites/default/files/pubs/2016-03_Single-pass-Parallel-Prefix/nvr-2016-002.pdf
// apparently.
void append(
    NumMeshletsPrefixSumResultBuffer buf,
    uint32_t instance_index,
    uint32_t num_meshlets
) {
    uint64_t upper_bits_one = uint64_t(1) << uint64_t(32);

    uint64_t add = uint64_t(num_meshlets) | upper_bits_one;
    uint64_t add_result = atomicAdd(buf.counter, add);

    uint32_t index = uint32_t(add_result >> 32);

    NumMeshletsPrefixSumResult result;
    result.instance_index = instance_index;
    // Add the current number of meshlets to make in inclusive rather than
    // exclusive.
    result.meshlets_offset = uint32_t(add_result) + num_meshlets;
    buf.values[index] = result;
}

//comp

layout(local_size_x = 64) in;

void main() {
    uint32_t instance_index = gl_GlobalInvocationID.x;

    if (instance_index >= get_uniforms().num_instances) {
        return;
    }

    Instance instance =
        InstanceBuffer(get_uniforms().instances).instances[instance_index];
    MeshInfo mesh_info = MeshInfoBuffer(instance.mesh_info_address).mesh_info;

    append(
        NumMeshletsPrefixSumResultBuffer(get_uniforms().num_meshlets_prefix_sum
        ),
        instance_index,
        mesh_info.num_meshlets
    );
}
