#include "bindings.hlsl"
#include "matrices.hlsl"

[shader("compute")]
[numthreads(8, 8, 1)]
void read_depth(uint3 global_id: SV_DispatchThreadID){
    uint width;
    uint height;
    depth_buffer.GetDimensions(width, height);
    float2 pixel_size = 1.0 / float2(uint2(width, height));

    float2 coord = float2(global_id.xy) + 0.5;
    float2 uv = coord * pixel_size;

    float depth = depth_buffer.SampleLevel(clamp_sampler, uv, 0.0);
    uint depth_reinterpreted = asuint(depth);

    if (depth_reinterpreted == 0) {
        return;
    }

    uint subgroup_min = WaveActiveMin(depth_reinterpreted);
    uint subgroup_max = WaveActiveMax(depth_reinterpreted);

    // https://www.khronos.org/assets/uploads/developers/library/2018-vulkan-devday/06-subgroups.pdf
    // equiv of subgroup elect
    if (WaveIsFirstLane()) {
        InterlockedMin(depth_info[0].min_depth, subgroup_min);
        InterlockedMax(depth_info[0].max_depth, subgroup_max);
    }
}

[shader("compute")]
[numthreads(4, 1, 1)]
void generate_matrices(uint3 global_id: SV_DispatchThreadID)
{
    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp#L650-L663

    DepthInfoBuffer info = depth_info[0];


    float min_depth = asfloat(info.min_depth);
    float max_depth = asfloat(info.max_depth);


    uint cascade_index = global_id.x;

    float cascadeSplits[4];

    float nearClip = 0.01;
    float farClip = 100000.0;
    float clipRange = farClip - nearClip;

    float minZ = nearClip + min_depth * clipRange;
    float maxZ = nearClip + max_depth * clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    float4x4 invCam = uniforms.inv_perspective_view;

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

    float lastSplitDist = min_depth;//(cascade_index > 0 ? cascadeSplits[cascade_index - 1] : min_depth);
    float splitDist = max_depth;//cascadeSplits[cascade_index];

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

    float3 lightDir = normalize(-SUN_DIR);

    float4x4 shadowView = lookAt(lightDir * minExtents.z + frustumCenter, frustumCenter, float3(0,1,0));
	float4x4 shadowProj = OrthographicProjection(minExtents.x, minExtents.y, maxExtents.x, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

    uint sMapSize = 1024;

    if (uniforms.stabilize_cascades) {
        // Create the rounding matrix, by projecting the world-space origin and determining
        // the fractional offset in texel space
        float4x4 shadowMatrix = mul(shadowView, shadowProj);
        float3 shadowOrigin = 0.0f;
        shadowOrigin = mul(float4(shadowOrigin, 1.0f), shadowMatrix).xyz;
        shadowOrigin *= (sMapSize / 2.0f);

        float3 roundedOrigin = round(shadowOrigin);
        float3 roundOffset = roundedOrigin - shadowOrigin;
        roundOffset = roundOffset * (2.0f / sMapSize);
        roundOffset.z = 0.0f;

        shadowProj[3][0] += roundOffset.x;
        shadowProj[3][1] += roundOffset.y;
    }

    depth_info[0].shadow_rendering_matrices[cascade_index] = mul(shadowProj, shadowView);
}
