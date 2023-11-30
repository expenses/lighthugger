#include "../common/bindings.hlsl"
#include "../common/matrices.hlsl"

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
