#include <shared_cpu_gpu.h>

[[vk::push_constant]]
CopyQuantizedPositionsConstant copy;

[shader("compute")]
[numthreads(64, 1, 1)]
void copy_quantized_positions(uint3 global_id: SV_DispatchThreadID) {
    uint32_t index = global_id.x;

    if (index >= copy.count) {
        return;
    }

    // load 8 x u32, write 6 x u32.
    uint16_t4 value = vk::RawBufferLoad<uint16_t4>(copy.src + sizeof(uint16_t4) * index);
    // Alignment of 2 is required here.
    vk::RawBufferStore<uint16_t3>(copy.dst + sizeof(uint16_t3) * index, value.xyz, 2);
}
