
layout(buffer_reference, scalar) buffer MeshInfoBuffer {
    MeshInfo mesh_info;
};

layout(buffer_reference, scalar) buffer MeshletReferenceBuffer {
    MeshletReference meshlet_reference[];
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

layout(buffer_reference, scalar, buffer_reference_align = 1) buffer
    QuanitizedNormals {
    int8_t3 normals[];
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

layout(buffer_reference, scalar) buffer UniformsBuffer {
    Uniforms uniforms;
};

layout(buffer_reference, scalar) buffer DispatchCommandsBuffer {
    DispatchIndirectCommand commands[];
};
