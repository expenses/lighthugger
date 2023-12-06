
layout(buffer_reference, scalar) buffer MeshInfoBuffer {
    MeshInfo mesh_info;
};

layout(buffer_reference, scalar) buffer MeshletIndexBuffer {
    MeshletIndex meshlet_index[];
};

layout(buffer_reference, scalar) buffer MeshletBuffer {
    Meshlet meshlets[];
};

layout(buffer_reference, scalar, buffer_reference_align = 1) buffer
    MicroIndexBuffer {
    u8vec3 indices[];
};

layout(buffer_reference, scalar, buffer_reference_align = 1) buffer
    MicroIndexBufferSingle {
    uint8_t indices[];
};

layout(buffer_reference, scalar) buffer Index32Buffer {
    uint32_t indices[];
};

layout(buffer_reference, scalar, buffer_reference_align = 2) buffer
    Index16Buffer {
    uint16_t indices[];
};

layout(buffer_reference, scalar, buffer_reference_align = 2) buffer
    QuantizedPositionBuffer {
    uint16_t3 positions[];
};

layout(buffer_reference, scalar) buffer QuanitizedUvs {
    uint16_t2 uvs[];
};

// todo: can change this to int8_t3 now that everything is in glsl.
layout(buffer_reference, scalar) buffer QuanitizedNormals {
    int8_t4 normals[];
};

layout(buffer_reference, scalar) buffer InstanceBuffer {
    Instance instances[];
};

layout(buffer_reference, scalar) buffer DrawCallBuffer {
    uint32_t4 num_opaque;
    uint32_t4 num_alpha_clip;
    DrawIndirectCommand draw_calls[];
};

layout(buffer_reference, scalar) buffer MiscStorageBuffer {
    MiscStorage misc_storage;
};

layout(buffer_reference, scalar) buffer PrefixSumValues {
    uint32_t values[];
};
