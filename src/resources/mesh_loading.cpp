#include "mesh_loading.h"

#include "../allocations/staging.h"
#include "image_loading.h"

Mesh load_obj(
    const char* filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers,
    DescriptorSet& descriptor_set
) {
    tinyobj::ObjReader reader;

    assert(reader.ParseFromFile(filepath));
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    auto str_filepath = std::string(filepath);

    std::vector<ImageWithView> images;

    auto base_path = str_filepath.substr(0, str_filepath.rfind("/") + 1);

    for (auto& material : materials) {
        auto albedo_texture_filepath = base_path + material.diffuse_texname;
        auto albedo_image = load_dds(
            albedo_texture_filepath.data(),
            allocator,
            device,
            command_buffer,
            graphics_queue_family,
            temp_buffers
        );
        descriptor_set.write_image(albedo_image, *device);
        images.push_back(std::move(albedo_image));
    }

    //dbg(attrib.vertices.size(), attrib.texcoords.size());

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

    float bounding_sphere_radius = 0.0;
    for (uint32_t i = 0; i < attrib.vertices.size() / 3; i++) {
        auto position = glm::vec3(
            attrib.vertices[i * 3],
            attrib.vertices[i * 3 + 1],
            attrib.vertices[i * 3 + 2]
        );
        bounding_sphere_radius =
            std::max(bounding_sphere_radius, glm::length2(position));
    }

    // Todo: should use staging buffers instead of host-accessible storage buffers.

    auto index_buffer = upload_via_staging_buffer(
        indices.data(),
        indices.size() * sizeof(uint32_t),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " index buffer",
        command_buffer,
        temp_buffers
    );

    auto position_buffer = upload_via_staging_buffer(
        attrib.vertices.data(),
        attrib.vertices.size() * sizeof(float),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " position buffer",
        command_buffer,
        temp_buffers
    );

    auto normal_buffer = upload_via_staging_buffer(
        attrib.normals.data(),
        attrib.normals.size() * sizeof(float),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " normal buffer",
        command_buffer,
        temp_buffers
    );

    auto uv_buffer = upload_via_staging_buffer(
        attrib.texcoords.data(),
        attrib.texcoords.size() * sizeof(float),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " uv buffer",
        command_buffer,
        temp_buffers
    );

    auto material_id_buffer = upload_via_staging_buffer(
        material_ids.data(),
        material_ids.size() * sizeof(uint32_t),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " material id buffer",
        command_buffer,
        temp_buffers
    );

    return {
        .positions = std::move(position_buffer),
        .indices = std::move(index_buffer),
        .normals = std::move(normal_buffer),
        .material_ids = std::move(material_id_buffer),
        .uvs = std::move(uv_buffer),
        .images = std::move(images),
        .num_indices = static_cast<uint32_t>(indices.size()),
        .bounding_sphere_radius = bounding_sphere_radius};
}

MeshBufferAddresses Mesh::get_addresses(const vk::raii::Device& device) {
    return {
        .positions = device.getBufferAddress({.buffer = positions.buffer}),
        .indices = device.getBufferAddress({.buffer = indices.buffer}),
        .normals = device.getBufferAddress({.buffer = normals.buffer}),
        .uvs = device.getBufferAddress({.buffer = uvs.buffer}),
        .material_indices =
            device.getBufferAddress({.buffer = material_ids.buffer})};
}
