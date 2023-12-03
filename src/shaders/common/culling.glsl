
bool cull_bounding_sphere(Instance instance, vec4 bounding_sphere) {
    vec3 world_space_pos =
        (instance.transform * vec4(bounding_sphere.xyz, 1.0)).xyz;
    float radius = bounding_sphere.w;

    vec3 scale = vec3(
        length(instance.transform[0].xyz),
        length(instance.transform[1].xyz),
        length(instance.transform[2].xyz)
    );
    // 99% of the time scales will be uniform. But in the chance they're not,
    // use the longest dimension.
    float scale_scalar = max(max(scale.x, scale.y), scale.z);

    radius *= scale_scalar;

    vec3 view_space_pos =
        (uniforms.initial_view * vec4(world_space_pos, 1.0)).xyz;
    // The view space goes from negatives in the front to positives in the back.
    // This is confusing so flipping it here makes sense I think.
    view_space_pos.z = -view_space_pos.z;

    // Is the most positive/forwards point of the object in front of the near plane?
    bool visible = view_space_pos.z + radius > NEAR_PLANE;

    // Do some fancy stuff by getting the frustum planes and comparing the position against them.
    vec3 frustum_x = normalize(
        transpose(uniforms.perspective)[3].xyz
        + transpose(uniforms.perspective)[0].xyz
    );
    vec3 frustum_y = normalize(
        transpose(uniforms.perspective)[3].xyz
        + transpose(uniforms.perspective)[1].xyz
    );

    visible = visible
        && view_space_pos.z * frustum_x.z + abs(view_space_pos.x) * frustum_x.x
            < radius;
    visible = visible
        && view_space_pos.z * frustum_y.z - abs(view_space_pos.y) * frustum_y.y
            < radius;

    return !visible;
}

bool cull_bounding_sphere_shadows(Instance instance, vec4 bounding_sphere) {
    MiscStorageBuffer buf = MiscStorageBuffer(uniforms.misc_storage);

    vec3 world_space_pos =
        (instance.transform * vec4(bounding_sphere.xyz, 1.0)).xyz;
    float radius = bounding_sphere.w;

    vec3 scale = vec3(
        length(instance.transform[0].xyz),
        length(instance.transform[1].xyz),
        length(instance.transform[2].xyz)
    );
    // 99% of the time scales will be uniform. But in the chance they're not,
    // use the longest dimension.
    float scale_scalar = max(max(scale.x, scale.y), scale.z);

    radius *= scale_scalar;

    vec3 view_space_pos = (buf.misc_storage.largest_cascade_view_matrix
                           * vec4(world_space_pos, 1.0))
                              .xyz;
    // The view space goes from negatives in the front to positives in the back.
    // This is confusing so flipping it here makes sense I think.
    view_space_pos.z = -view_space_pos.z;

    // Is the most positive/forwards point of the object in front of the near plane?
    bool visible = view_space_pos.z + radius > NEAR_PLANE;

    visible = visible
        && abs(view_space_pos.x) - radius
            < buf.misc_storage.shadow_sphere_radius;
    visible = visible
        && abs(view_space_pos.y) - radius
            < buf.misc_storage.shadow_sphere_radius;
    return !visible;
}

bool cull_cone_perspective(Instance instance, Meshlet meshlet) {
    float3 apex = (instance.transform * float4(meshlet.cone_apex, 1.0)).xyz;
    float3 axis = normalize((instance.normal_transform * meshlet.cone_axis));

    return dot(normalize(apex - uniforms.camera_pos), normalize(axis))
        >= meshlet.cone_cutoff;
}

bool cull_cone_orthographic(Instance instance, Meshlet meshlet) {
    float3 axis = normalize((instance.normal_transform * meshlet.cone_axis));
    return dot(uniforms.sun_dir, axis) >= meshlet.cone_cutoff;
}
