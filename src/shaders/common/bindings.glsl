#include "hlsl4glsl.glsl"
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

layout(binding = 11, scalar) buffer DrawCallBuffer {
    DrawIndirectCommand draw_calls[];
};

layout(buffer_reference, scalar) buffer MeshInfoBuffer {
    MeshInfo mesh_info;
};

layout(buffer_reference, scalar) buffer MeshletIndexBuffer {
    MeshletIndex meshlet_index[];
};

layout(buffer_reference, scalar) buffer MeshletBuffer {
    Meshlet meshlets[];
};
