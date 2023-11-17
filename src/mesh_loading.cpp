#include "mesh_loading.h"

template<class T>
AllocatedBuffer allocate_from_vector(
    const std::vector<T>& vector,
    vma::Allocator allocator,
    const std::string& name
) {
    AllocatedBuffer buffer(
        vk::BufferCreateInfo {
            .size = vector.size() * sizeof(T),
            .usage = vk::BufferUsageFlagBits::eTransferSrc
                | vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eShaderDeviceAddress},
        {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator,
        name.data()
    );

    buffer.map_and_memcpy((void*)vector.data(), vector.size() * sizeof(T));

    return buffer;
}

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

    std::vector<uint32_t> indices;
    std::vector<uint32_t> material_ids;
    for (auto& shape : shapes) {
        for (auto& index : shape.mesh.indices) {
            assert(index.vertex_index == index.normal_index);
            indices.push_back(index.vertex_index);
        }
        for (uint32_t material_id : shape.mesh.material_ids) {
            material_ids.push_back(material_id);
            material_ids.push_back(material_id);
            material_ids.push_back(material_id);
        }
    }

    std::cout << indices.size() << "; " << material_ids.size() << "; "
              << attrib.vertices.size() << std::endl;

    // Todo: should use staging buffers instead of host-accessible storage buffers.

    auto index_buffer = allocate_from_vector(
        indices,
        allocator,
        std::string(filepath) + " index buffer"
    );

    auto vertex_buffer = allocate_from_vector(
        attrib.vertices,
        allocator,
        std::string(filepath) + " vertex buffer"
    );

    auto normal_buffer = allocate_from_vector(
        attrib.normals,
        allocator,
        std::string(filepath) + " normal buffer"
    );

    auto material_id_buffer = allocate_from_vector(
        material_ids,
        allocator,
        std::string(filepath) + " material id buffer"
    );

    return Mesh(
        std::move(vertex_buffer),
        std::move(index_buffer),
        std::move(normal_buffer),
        std::move(material_id_buffer),
        indices.size()
    );
}

MeshBufferAddresses Mesh::get_addresses(const vk::raii::Device& device) {
    return {
        .positions = device.getBufferAddress({.buffer = vertices.buffer}),
        .indices = device.getBufferAddress({.buffer = indices.buffer}),
        .normals = device.getBufferAddress({.buffer = normals.buffer}),
        .material_ids =
            device.getBufferAddress({.buffer = material_ids.buffer})};
}
