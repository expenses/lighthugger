#include "allocations.h"

struct Mesh {
    AllocatedBuffer vertices;
    AllocatedBuffer indices;
    uint32_t num_indices;
};

Mesh load_obj(const char* filepath, vma::Allocator allocator);
