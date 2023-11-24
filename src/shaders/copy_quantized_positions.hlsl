#include "../shared_cpu_gpu.h"

[[vk::push_constant]]
CopyQuantizedPositionsConstant copy;

uint16_t3 unpack(uint16_t2 packed) {
    uint16_t3 unpacked;
    unpacked.x = packed.x & 255;
    unpacked.y = packed.x >> 8;
    unpacked.z = packed.y & 255;
    // packed.y >> 8 is a padding byte.
    return unpacked;
}

uint16_t2 pack(uint16_t4 value) {
    uint16_t2 packed;
    packed.x = value.x | value.y << 8;
    packed.y = value.z | value.w << 8;
    return packed;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void copy_quantized_positions(uint3 global_id: SV_DispatchThreadID) {
    uint32_t index = global_id.x;

    if (index >= copy.count) {
        return;
    }

    if (copy.use_16bit) {
        // load 8 x u32, write 6 x u32.
        uint16_t4 value = vk::RawBufferLoad<uint16_t4>(copy.src + sizeof(uint16_t4) * index);
        // Alignment of 2 is required here.
        vk::RawBufferStore<uint16_t3>(copy.dst + sizeof(uint16_t3) * index, value.xyz, 2);
    } else {
        // load 4 x u32, write 3 x u32. Using 16-bit types for (relative) convenience.
        // I wrote this intending to use it for packing quantized 3 x i8 normals together,
        // but I ran into problems with alignment and other stuff and decided that a saving of 1 byte
        // per-normal wasn't worth it.
        uint16_t3 value_0 = unpack(vk::RawBufferLoad<uint16_t2>(copy.src + sizeof(uint16_t2) * (index * 4 + 0)));
        uint16_t3 value_1 = unpack(vk::RawBufferLoad<uint16_t2>(copy.src + sizeof(uint16_t2) * (index * 4 + 1)));
        uint16_t3 value_2 = unpack(vk::RawBufferLoad<uint16_t2>(copy.src + sizeof(uint16_t2) * (index * 4 + 2)));
        uint16_t3 value_3 = unpack(vk::RawBufferLoad<uint16_t2>(copy.src + sizeof(uint16_t2) * (index * 4 + 3)));

        // Uncomment for testing.
        //vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 4 + 0), pack(uint16_t4(value_0, 0)));
        //vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 4 + 1), pack(uint16_t4(value_1, 0)));
        //vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 4 + 2), pack(uint16_t4(value_2, 0)));
        //vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 4 + 3), pack(uint16_t4(value_3, 0)));

        vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 3 + 0), pack(uint16_t4(value_0, value_1.x)), 1);
        vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 3 + 1), pack(uint16_t4(value_1.yz, value_2.xy)), 1);
        vk::RawBufferStore<uint16_t2>(copy.dst + sizeof(uint16_t2) * (index * 3 + 2), pack(uint16_t4(value_1.z, value_3)), 1);
    }
}
