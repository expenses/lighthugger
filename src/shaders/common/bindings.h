#include "hlsl4glsl.h"
#include <shared_cpu_gpu.h>

// Ensure that all of these are scalar!

layout(binding = 1, scalar) buffer InstanceBuffer {
    Instance instances[];
};

layout(binding = 2, scalar) uniform UniformsBinding {
    Uniforms uniforms;
};

layout(binding = 8, scalar) buffer MiscStorageBuffer {
    MiscStorage misc_storage;
};

layout(buffer_reference, scalar) buffer MeshInfoBuffer {
    MeshInfo mesh_info;
};

layout(buffer_reference, scalar) buffer MeshletIndexBuffer {
    MeshletIndex meshlet_index[];
};
