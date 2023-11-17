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
    tinyobj::ObjReader reader;

    assert(reader.ParseFromFile(filepath));
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    auto str_filepath = std::string(filepath);

    auto m = materials[0];
    auto material_filepath = str_filepath.substr(0, str_filepath.rfind("/") + 1) + m.diffuse_texname;

    std::vector<uint32_t> indices;
    std::vector<uint32_t> material_ids;
    for (auto& shape : shapes) {
        for (auto& index : shape.mesh.indices) {
            assert(index.vertex_index == index.normal_index);
            indices.push_back(index.vertex_index);
        }
        // :( material ids are stored unindexed.
        for (uint32_t material_id : shape.mesh.material_ids) {
            material_ids.push_back(material_id);
            material_ids.push_back(material_id);
            material_ids.push_back(material_id);
        }
    }


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
        .material_indices =
            device.getBufferAddress({.buffer = material_ids.buffer})};
}
