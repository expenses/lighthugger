#include "../common/bindings.glsl"

uint32_t min_if_not_zero(uint32_t a, uint32_t b) {
    // Use HLSL 2021's `select` instead of `?:` to avoid branches.
    // Not sure if it matters.
    return select(a != 0u, select(b != 0u, min(a, b), a), b);
}

uint32_t max4(uint4 values) {
    return max(max(values.x, values.y), max(values.z, values.w));
}

uint32_t min4(uint4 values) {
    return min_if_not_zero(
        min_if_not_zero(values.x, values.y),
        min_if_not_zero(values.z, values.w)
    );
}

layout(local_size_x = 8, local_size_y = 8) in;

void read_depth() {
    uint3 global_id = gl_GlobalInvocationID;
    if (global_id.x >= uniforms.window_size.x
        || global_id.y >= uniforms.window_size.y) {
        return;
    }

    float2 pixel_size = 1.0 / float2(uniforms.window_size);

    // Sample the depth values for a 4x4 block.
    uint2 coord = global_id.xy * 4;

    uint4 depth_1 = asuint(textureGather(
        sampler2D(depth_buffer, clamp_sampler),
        (coord + uint2(1, 1)) * pixel_size
    ));
    uint4 depth_2 = asuint(textureGather(
        sampler2D(depth_buffer, clamp_sampler),
        (coord + uint2(1, 3)) * pixel_size
    ));
    uint4 depth_3 = asuint(textureGather(
        sampler2D(depth_buffer, clamp_sampler),
        (coord + uint2(3, 1)) * pixel_size
    ));
    uint4 depth_4 = asuint(textureGather(
        sampler2D(depth_buffer, clamp_sampler),
        (coord + uint2(3, 3)) * pixel_size
    ));

    // min the values, trying to avoid propagating zeros.
    uint32_t depth_min =
        min4(uint4(min4(depth_1), min4(depth_2), min4(depth_3), min4(depth_4)));

    if (depth_min != 0) {
        // Min all values in the subgroup,
        uint32_t subgroup_min = subgroupMin(depth_min);

        // https://www.khronos.org/assets/uploads/developers/library/2018-vulkan-devday/06-subgroups.pdf
        // equiv of subgroup elect
        // Note: naming is ambiguous but this means the first
        // _active_ lane.
        if (subgroupElect()) {
            atomicMin(misc_storage.min_depth, subgroup_min);
        }
    }

    uint32_t depth_max =
        max4(uint4(max4(depth_1), max4(depth_2), max4(depth_3), max4(depth_4)));
    uint32_t subgroup_max = subgroupMax(depth_max);

    if (subgroupElect()) {
        atomicMax(misc_storage.max_depth, subgroup_max);
    }
}
