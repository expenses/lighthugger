#include "../common/bindings.glsl"
#include "../common/util.glsl"

layout(local_size_x = 1) in;

const uint32_t UINT32_T_MAX_VALUE = 4294967295;

// Before the prefix sum.
void reset_buffers_a() {
    Uniforms uniforms = get_uniforms();
    MiscStorageBuffer buf = MiscStorageBuffer(uniforms.misc_storage);
    buf.misc_storage.min_depth = UINT32_T_MAX_VALUE;
    buf.misc_storage.max_depth = 0;

    DispatchCommandsBuffer dispatches =
        DispatchCommandsBuffer(get_uniforms().dispatches);

    dispatches.commands[PER_INSTANCE_DISPATCH].x =
        dispatch_size(uniforms.num_instances, 64);
    dispatches.commands[PER_INSTANCE_DISPATCH].y = 1;
    dispatches.commands[PER_INSTANCE_DISPATCH].z = 1;

    dispatches.commands[PER_SHADOW_INSTANCE_DISPATCH].x =
        dispatch_size(uniforms.num_instances, 16);
    dispatches.commands[PER_SHADOW_INSTANCE_DISPATCH].y = 1;
    dispatches.commands[PER_SHADOW_INSTANCE_DISPATCH].z = 1;

    reset_counters();
}

// After the prefix sum.
void reset_buffers_b() {
    DispatchCommandsBuffer dispatches =
        DispatchCommandsBuffer(get_uniforms().dispatches);

    dispatches.commands[PER_MESHLET_DISPATCH].x =
        dispatch_size(total_num_meshlets_for_cascade(0), 64);
    dispatches.commands[PER_MESHLET_DISPATCH].y = 1;
    dispatches.commands[PER_MESHLET_DISPATCH].z = 1;
}

// After the shadow prefix sum.
void reset_buffers_c() {
    DispatchCommandsBuffer dispatches =
        DispatchCommandsBuffer(get_uniforms().dispatches);

    dispatches.commands[PER_MESHLET_DISPATCH].x = dispatch_size(
        total_num_meshlets_for_cascade(shadow_constant.cascade_index),
        64
    );
    dispatches.commands[PER_MESHLET_DISPATCH].y = 1;
    dispatches.commands[PER_MESHLET_DISPATCH].z = 1;

    DrawCallBuffer draw_call_buf = DrawCallBuffer(get_uniforms().draw_calls);
    draw_call_buf.num_opaque = 0;
    draw_call_buf.num_alpha_clip = 0;
}
