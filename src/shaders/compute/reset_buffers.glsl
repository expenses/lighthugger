#include "../common/bindings.glsl"
#include "../common/util.glsl"

layout(local_size_x = 1) in;

const uint32_t UINT32_T_MAX_VALUE = 4294967295;

void reset_buffers() {
    Uniforms uniforms = get_uniforms();
    MiscStorageBuffer buf = MiscStorageBuffer(uniforms.misc_storage);
    buf.misc_storage.min_depth = UINT32_T_MAX_VALUE;
    buf.misc_storage.max_depth = 0;

    reset_draw_calls();

    buf.misc_storage.per_meshlet_dispatch.x =
        dispatch_size(total_num_meshlets(), 64);
    buf.misc_storage.per_meshlet_dispatch.y = 1;
    buf.misc_storage.per_meshlet_dispatch.z = 1;

    buf.misc_storage.per_shadow_meshlet_dispatch.x =
        dispatch_size(total_num_meshlets(), 16);
    buf.misc_storage.per_shadow_meshlet_dispatch.y = 1;
    buf.misc_storage.per_shadow_meshlet_dispatch.z = 1;
}
