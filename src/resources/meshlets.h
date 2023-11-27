#include "../shared_cpu_gpu.h"

struct Meshlets {
    std::vector<Meshlet> meshlets;
    // Contains a micro index buffer that indexes into the main index buffer.
    // Yeah it's confusing.
    std::vector<uint8_t> meshlet_triangles;

    // This is the new index buffer.
    std::vector<uint32_t> meshlet_indices_32bit;
    std::vector<uint16_t> meshlet_indices_16bit;
};

Meshlets build_meshlets(
    uint8_t* indices,
    size_t indices_count,
    float* positions,
    size_t vertices_count,
    bool uses_32_bit_indices
);
