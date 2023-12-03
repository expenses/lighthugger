#include "../common/bindings.glsl"

layout(local_size_x = 1) in;

const uint32_t UINT32_T_MAX_VALUE = 4294967295;

void reset_buffers() {
    MiscStorageBuffer buf = MiscStorageBuffer(uniforms.misc_storage);
    buf.misc_storage.min_depth = UINT32_T_MAX_VALUE;
    buf.misc_storage.max_depth = 0;
    buf.misc_storage.num_expanded_meshlets = 0;

    DrawCallBuffer draw_call_buf = DrawCallBuffer(uniforms.draw_calls);
    draw_call_buf.num_opaque = 0;
    draw_call_buf.num_alpha_clip = 0;
}

void reset_draw_calls() {
    DrawCallBuffer draw_call_buf = DrawCallBuffer(uniforms.draw_calls);
    draw_call_buf.num_opaque = 0;
    draw_call_buf.num_alpha_clip = 0;
}
