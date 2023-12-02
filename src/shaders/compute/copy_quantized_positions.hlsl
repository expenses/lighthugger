#include "../common/hlsl4glsl.glsl"
#include <shared_cpu_gpu.h>

layout(push_constant) uniform PushConstant {
    CopyQuantizedPositionsConstant copy;
};

layout(buffer_reference, scalar) buffer Uint16_t4 {
    uint16_t4 values[];
};

// Alignment of 2 is required here.
layout(buffer_reference, scalar, buffer_reference_align = 2) buffer Uint16_t3 {
    uint16_t3 values[];
};

layout(local_size_x = 64) in;
void copy_quantized_positions() {
    uint32_t index = gl_GlobalInvocationID.x;

    if (index >= copy.count) {
        return;
    }

    // load 8 x u32, write 6 x u32.
    uint16_t4 value = Uint16_t4(copy.src).values[index];
    Uint16_t3(copy.dst).values[index] = value.xyz;
}
