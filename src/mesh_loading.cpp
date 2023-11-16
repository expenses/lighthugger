#include "mesh_loading.h"

Mesh load_obj(const char* filepath, vma::Allocator allocator) {
    tinyobj::ObjReaderConfig reader_config;
    tinyobj::ObjReader reader;

    assert(reader.ParseFromFile(filepath, reader_config));
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    std::cout << shapes.size() << " " << materials.size() << std::endl;

    for (auto& shape : shapes) {
        // Check that all geometry is triangulated
        for (auto num_face_vertices : shape.mesh.num_face_vertices) {
            assert(num_face_vertices == 3);
        }
    }

    auto& shape = shapes[0];

    std::vector<uint32_t> indices(shape.mesh.indices.size());
    for (auto& index : shape.mesh.indices) {
        indices.push_back(index.vertex_index);
    }

    // Todo: should use staging buffers instead of host-accessible storage buffers.

    std::string index_buffer_name = std::string(filepath) + " index buffer";

    AllocatedBuffer index_buffer(
        vk::BufferCreateInfo {
            .size = indices.size() * sizeof(uint32_t),
            .usage = vk::BufferUsageFlagBits::eTransferSrc
                | vk::BufferUsageFlagBits::eStorageBuffer},
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator,
        index_buffer_name.data()
    );

    std::string vertex_buffer_name = std::string(filepath) + " vertex buffer";
    auto vertex_buffer = AllocatedBuffer(
        {.size = attrib.vertices.size() * sizeof(float),
         .usage = vk::BufferUsageFlagBits::eTransferSrc
             | vk::BufferUsageFlagBits::eStorageBuffer},
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator,
        vertex_buffer_name.data()
    );

    vertex_buffer.map_and_memcpy(
        (void*)attrib.vertices.data(),
        attrib.vertices.size() * sizeof(float)
    );

    return Mesh {
        .vertices = std::move(vertex_buffer),
        .indices = std::move(index_buffer),
        .num_indices = indices.size()};
}
