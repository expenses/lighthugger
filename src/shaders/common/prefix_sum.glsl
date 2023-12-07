//struct PrefixSumValue {
//    uint32_t index;
//    uint32_t sum;
//};

layout(buffer_reference, scalar) buffer PrefixSumBuffer {
    uint64_t counter;
    PrefixSumValue values[];
};

// Prefix sum for 32-bit values using 64-bit atomics.
// Super simple, inspired by Nabla's implementation:
// https://github.com/Devsh-Graphics-Programming/Nabla/blob/8da4b980b5617802ea4e96bf101ddbfe94721a51/include/nbl/builtin/glsl/scanning_append/scanning_append.glsl
//
// See also https://research.nvidia.com/sites/default/files/pubs/2016-03_Single-pass-Parallel-Prefix/nvr-2016-002.pdf
// apparently.
void prefix_sum_inclusive_append(
    PrefixSumBuffer buf,
    uint32_t index,
    uint32_t value
) {
    uint64_t upper_bits_one = uint64_t(1) << uint64_t(32);
    uint64_t sum_and_counter =
        atomicAdd(buf.counter, uint64_t(value) | upper_bits_one);
    uint32_t buffer_index = uint32_t(sum_and_counter >> 32);

    buf.values[buffer_index].index = index;
    // Add the current value to make in inclusive rather than exclusive.
    buf.values[buffer_index].sum = uint32_t(sum_and_counter) + value;
}

// See https://en.cppreference.com/w/cpp/algorithm/upper_bound.
PrefixSumValue prefix_sum_binary_search(PrefixSumBuffer buf, uint32_t target) {
    uint32_t count = uint32_t(buf.counter >> 32);
    uint32_t first = 0;

    while (count > 0) {
        uint32_t step = (count / 2);
        uint32_t current = first + step;
        bool greater = target >= buf.values[current].sum;
        first = select(greater, current + 1, first);
        count = select(greater, count - (step + 1), step);
    }

    return buf.values[first];
}

void prefix_sum_reset(PrefixSumBuffer buf) {
    buf.counter = 0;
}

uint32_t prefix_sum_total(PrefixSumBuffer buf) {
    return uint32_t(buf.counter);
}
