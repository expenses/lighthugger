#include "common/bindings.hlsl"
#include "common/matrices.hlsl"

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
    uint32_t width;
    uint32_t height;
    depth_buffer.GetDimensions(width, height);
    float2 pixel_size = 1.0 / float2(width, height);

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
            InterlockedMin(depth_info[0].min_depth, subgroup_min);
        }
    }

    uint32_t depth_max = max4(uint4(max4(depth_1), max4(depth_2), max4(depth_3), max4(depth_4)));
    uint32_t subgroup_max = WaveActiveMax(depth_max);

    if (WaveIsFirstLane()) {
        InterlockedMax(depth_info[0].max_depth, subgroup_max);
    }
}

[shader("compute")]
[numthreads(4, 1, 1)]
void generate_matrices(uint3 global_id: SV_DispatchThreadID)
{
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp#L650-L663

    DepthInfoBuffer info = depth_info[0];

    // Note: these values are misleading. As 0.0 is the infinite plane,
    // the max_depth is actually the closer value!
    float min_depth = asfloat(info.min_depth);
    float max_depth = asfloat(info.max_depth);

    float4x4 invCam = uniforms.inv_perspective_view;

    uint32_t cascade_index = global_id.x;

    //float cascadeSplits[4];
    /*
    float nearClip = 0.01;
    float farClip = 100000.0;
    float clipRange = farClip - nearClip;

    float minZ = nearClip + min_depth * clipRange;
    float maxZ = nearClip + max_depth * clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;


    float cascadeSplitLambda = 0.95;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < 4; i++) {
            float p = (i + 1) / float(4);
            float log = minZ * pow(ratio, p);
            float uniform_value = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform_value) + uniform_value;
            cascadeSplits[i] = (d - nearClip) / clipRange;
    }
    */

    float cascadeSplits[4] = {
        lerp(max_depth, min_depth, 0.25), lerp(max_depth, min_depth, 0.5), lerp(max_depth, min_depth, 0.75), min_depth
    };

    float lastSplitDist = (cascade_index > 0 ? cascadeSplits[cascade_index - 1] : max_depth);
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

    // Set the camera to be a fixed distance away from the frustum center, so that
    // we don't get clipping on the near plane. We use the longest distance of the
    // (currently only) mesh in the scene for this.
    // Ideally you want to have the near and far plane be the distance of the closest
    // point on a mesh and the furthest point on a mesh respectively.
    float shadow_cam_distance = load_instance(0).mesh_info.bounding_sphere_radius * 2.0f;

    float4x4 shadowView = lookAt(shadow_cam_distance * uniforms.sun_dir + frustumCenter, frustumCenter, float3(0,1,0));
	float4x4 shadowProj = OrthographicProjection(minExtents.x, minExtents.y, maxExtents.x, maxExtents.y, 0.0f, shadow_cam_distance * 2.0);

    depth_info[0].shadow_rendering_matrices[cascade_index] = mul(shadowProj, shadowView);
}
