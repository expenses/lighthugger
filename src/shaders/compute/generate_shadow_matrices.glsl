#include "../common/bindings.glsl"
#include "../common/matrices.glsl"
#include "../common/util.glsl"

// Transforms the matrix output from NDC to UV space.
mat4 bias_matrix = mat4(
    vec4(0.5, 0.0, 0.0, 0.0),
    vec4(0.0, 0.5, 0.0, 0.0),
    vec4(0.0, 0.0, 1.0, 0.0),
    vec4(0.5, 0.5, 0.0, 1.0)
);

layout(local_size_x = 4) in;

void generate_shadow_matrices() {
    Uniforms uniforms = get_uniforms();

    MiscStorageBuffer buf = MiscStorageBuffer(uniforms.misc_storage);

    // https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp#L650-L663

    // Note: these values are misleading. As 0.0 is the infinite plane,
    // the max_depth is actually the closer value!
    float min_depth = asfloat(buf.misc_storage.min_depth);
    float max_depth = asfloat(buf.misc_storage.max_depth);

    float4x4 invCam = uniforms.inv_perspective_view;

    uint32_t cascade_index = gl_GlobalInvocationID.x;

    float min_distance = convert_infinite_reverze_z_depth(max_depth);
    float max_distance = convert_infinite_reverze_z_depth(min_depth);

    float cascadeSplits[4] = {
        // The first shadow frustum just tightly fits the aabb of the plane of the nearest stuff.
        // This is generally reasonably large.
        max_depth,
        convert_infinite_reverze_z_depth(lerp(
            min_distance,
            max_distance,
            pow(0.5, uniforms.cascade_split_pow)
        )),
        convert_infinite_reverze_z_depth(lerp(
            min_distance,
            max_distance,
            pow(0.75, uniforms.cascade_split_pow)
        )),
        min_depth};

    float lastSplitDist =
        select(cascade_index > 0, cascadeSplits[cascade_index - 1], max_depth);
    float splitDist = cascadeSplits[cascade_index];

    // Get the corners of the visible depth slice in view space
    float3 frustumCorners[8] = {
        float3(-1.0f, 1.0f, lastSplitDist),
        float3(1.0f, 1.0f, lastSplitDist),
        float3(1.0f, -1.0f, lastSplitDist),
        float3(-1.0f, -1.0f, lastSplitDist),
        float3(-1.0f, 1.0f, splitDist),
        float3(1.0f, 1.0f, splitDist),
        float3(1.0f, -1.0f, splitDist),
        float3(-1.0f, -1.0f, splitDist),
    };

    // Convert the corners to world space
    for (uint32_t i = 0; i < 8; i++) {
        float4 invCorner = (invCam * float4(frustumCorners[i], 1.0f));
        frustumCorners[i] = (invCorner / invCorner.w).xyz;
    }

    // Get frustum center
    float3 frustumCenter = float3(0.0f);
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

    float4x4 shadowView = lookAt(
        uniforms.shadow_cam_distance * uniforms.sun_dir + frustumCenter,
        frustumCenter,
        float3(0, 1, 0)
    );
    float4x4 shadowProj = OrthographicProjection(
        -sphere_radius,
        sphere_radius,
        -sphere_radius,
        sphere_radius,
        0.0f,
        uniforms.shadow_cam_distance * 2.0
    );

    mat4 matrix = shadowProj * shadowView;

    buf.misc_storage.shadow_matrices[cascade_index] = matrix;
    buf.misc_storage.uv_space_shadow_matrices[cascade_index] =
        bias_matrix * matrix;

    buf.misc_storage.shadow_view_matrices[cascade_index] = shadowView;
    buf.misc_storage.shadow_sphere_radii[cascade_index] = sphere_radius;

    reset_draw_calls();
}
