#include "common/bindings.hlsl"
#include "common/matrices.hlsl"
#include "common/loading.hlsl"
#include "common/geometry.hlsl"

#define FLT_MAX 3.402823466e+38
#define FLT_MIN 1.175494351e-38

uint32_t min_if_not_zero(uint32_t a, uint32_t b) {
    // Use HLSL 2021's `select` instead of `?:` to avoid branches.
    // Not sure if it matters.
    return select(a != 0u, select(b != 0u, min(a, b), a), b);
}

uint32_t max4(uint4 values) {
    return max(max(values.x, values.y), max(values.z, values.w));
}

uint32_t min4(uint4 values) {
    return min_if_not_zero(min_if_not_zero(values.x, values.y), min_if_not_zero(values.z, values.w));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void read_depth(uint3 global_id: SV_DispatchThreadID){
    if (global_id.x >= uniforms.window_size.x || global_id.y >= uniforms.window_size.y) {
        return;
    }

    float2 pixel_size = 1.0 / float2(uniforms.window_size);

    // Sample the depth values for a 4x4 block.
    uint2 coord = global_id.xy * 4;

    uint4 depth_1 = asuint(depth_buffer.Gather(clamp_sampler, (coord + uint2(1, 1)) * pixel_size, 0.0));
    uint4 depth_2 = asuint(depth_buffer.Gather(clamp_sampler, (coord + uint2(1, 3)) * pixel_size, 0.0));
    uint4 depth_3 = asuint(depth_buffer.Gather(clamp_sampler, (coord + uint2(3, 1)) * pixel_size, 0.0));
    uint4 depth_4 = asuint(depth_buffer.Gather(clamp_sampler, (coord + uint2(3, 3)) * pixel_size, 0.0));

    // min the values, trying to avoid propagating zeros.
    uint32_t depth_min = min4(uint4(min4(depth_1), min4(depth_2), min4(depth_3), min4(depth_4)));

    if (depth_min != 0) {
        // Min all values in the subgroup,
        uint32_t subgroup_min = WaveActiveMin(depth_min);

        // https://www.khronos.org/assets/uploads/developers/library/2018-vulkan-devday/06-subgroups.pdf
        // equiv of subgroup elect
        // Note: naming is ambiguous but this means the first
        // _active_ lane.
        if (WaveIsFirstLane()) {
            InterlockedMin(misc_storage[0].min_depth, subgroup_min);
        }
    }

    uint32_t depth_max = max4(uint4(max4(depth_1), max4(depth_2), max4(depth_3), max4(depth_4)));
    uint32_t subgroup_max = WaveActiveMax(depth_max);

    if (WaveIsFirstLane()) {
        InterlockedMax(misc_storage[0].max_depth, subgroup_max);
    }
}

// Super easy.
float convert_infinite_reverze_z_depth(float depth) {
    return NEAR_PLANE / depth;
}

[shader("compute")]
[numthreads(4, 1, 1)]
void generate_matrices(uint3 global_id: SV_DispatchThreadID)
{
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp#L650-L663

    // Note: these values are misleading. As 0.0 is the infinite plane,
    // the max_depth is actually the closer value!
    float min_depth = asfloat(misc_storage[0].min_depth);
    float max_depth = asfloat(misc_storage[0].max_depth);

    float4x4 invCam = uniforms.inv_perspective_view;

    uint32_t cascade_index = global_id.x;

    float min_distance = convert_infinite_reverze_z_depth(max_depth);
    float max_distance = convert_infinite_reverze_z_depth(min_depth);

    float cascadeSplits[4] = {
        // The first shadow frustum just tightly fits the aabb of the plane of the nearest stuff.
        // This is generally reasonably large.
        max_depth,
        convert_infinite_reverze_z_depth(lerp(min_distance, max_distance, pow(0.5, uniforms.cascade_split_pow))),
        convert_infinite_reverze_z_depth(lerp(min_distance, max_distance, pow(0.75, uniforms.cascade_split_pow))),
        min_depth
    };

    float lastSplitDist = select(cascade_index > 0, cascadeSplits[cascade_index - 1], max_depth);
    float splitDist = cascadeSplits[cascade_index];

    // Get the corners of the visible depth slice in view space
    float3 frustumCorners[8] = {
        float3(-1.0f,  1.0f, lastSplitDist),
        float3( 1.0f,  1.0f, lastSplitDist),
        float3( 1.0f, -1.0f, lastSplitDist),
        float3(-1.0f, -1.0f, lastSplitDist),
        float3(-1.0f,  1.0f, splitDist),
        float3( 1.0f,  1.0f, splitDist),
        float3( 1.0f, -1.0f, splitDist),
        float3(-1.0f, -1.0f, splitDist),
    };

    // Convert the corners to world space
    for (uint32_t i = 0; i < 8; i++) {
        float4 invCorner = mul(invCam, float4(frustumCorners[i], 1.0f));
        frustumCorners[i] = (invCorner / invCorner.w).xyz;
    }

    // Get frustum center
    float3 frustumCenter = 0.0f;
    for (uint32_t i = 0; i < 8; i++) {
        frustumCenter += frustumCorners[i];
    }
    frustumCenter /= 8.0f;

    float sphere_radius = 0.0f;
    for (uint32_t i = 0; i < 8; i++) {
        float distance = length(frustumCorners[i] - frustumCenter);
        sphere_radius = max(sphere_radius, distance);
    }
    sphere_radius = ceil(sphere_radius * 16.0f) / 16.0f;

    float3 maxExtents = sphere_radius;
    float3 minExtents = -maxExtents;

    float4x4 shadowView = lookAt(uniforms.shadow_cam_distance * uniforms.sun_dir + frustumCenter, frustumCenter, float3(0,1,0));
	float4x4 shadowProj = OrthographicProjection(minExtents.x, minExtents.y, maxExtents.x, maxExtents.y, 0.0f, uniforms.shadow_cam_distance * 2.0);

    misc_storage[0].shadow_matrices[cascade_index] = mul(shadowProj, shadowView);
}
