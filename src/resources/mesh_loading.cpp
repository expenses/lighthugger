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

    std::vector<uint32_t> image_indices;
    std::vector<MaterialInfo> material_info;
    std::unordered_map<std::string, uint32_t> material_filenames;

    for (auto& material : materials) {
        MaterialInfo info;

        if (material_filenames.contains(material.diffuse_texname)) {
            info.albedo_texture_index =
                material_filenames[material.diffuse_texname];
        } else {
            auto albedo_texture_filepath = base_path + material.diffuse_texname;
            auto albedo_image = load_dds(
                albedo_texture_filepath.data(),
                allocator,
                device,
                command_buffer,
                graphics_queue_family,
                temp_buffers
            );
            auto index = descriptor_set.write_image(albedo_image, *device);
            images.push_back(std::move(albedo_image));
            image_indices.push_back(index);
            info.albedo_texture_index = index;
            material_filenames[material.diffuse_texname] = index;
        }

        material_info.push_back(info);
    }

    assert(materials.size() <= 1 << 16);

    std::vector<uint32_t> indices;
    std::vector<uint16_t> material_ids(attrib.vertices.size() / 3);
    for (auto& shape : shapes) {
        for (auto& index : shape.mesh.indices) {
            assert(index.vertex_index == index.normal_index);
            indices.push_back(static_cast<uint32_t>(index.vertex_index));
        }

        // :( material ids are stored unindexed so we need to reindex them.
        // This could cause visual problems if a vertex is reused but with different materials.
        assert(shape.mesh.indices.size() / 3 == shape.mesh.material_ids.size());

        for (size_t i = 0; i < shape.mesh.material_ids.size(); i++) {
            auto material_id =
                static_cast<uint16_t>(shape.mesh.material_ids[i]);
            material_ids[shape.mesh.indices[i * 3].vertex_index] = material_id;
            material_ids[shape.mesh.indices[i * 3 + 1].vertex_index] =
                material_id;
            material_ids[shape.mesh.indices[i * 3 + 2].vertex_index] =
                material_id;
        }
    }

    BoundingBox bounding_box;

    float bounding_sphere_radius = 0.0;
    for (uint32_t i = 0; i < attrib.vertices.size() / 3; i++) {
        auto position = glm::vec3(
            attrib.vertices[i * 3],
            attrib.vertices[i * 3 + 1],
            attrib.vertices[i * 3 + 2]
        );
        bounding_sphere_radius =
            std::max(bounding_sphere_radius, glm::length2(position));

        bounding_box.insert(position);
    }
    bounding_sphere_radius = sqrtf(bounding_sphere_radius);

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
        material_ids.size() * sizeof(uint16_t),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " material id buffer",
        command_buffer,
        temp_buffers
    );

    auto material_info_buffer = upload_via_staging_buffer(
        material_info.data(),
        material_info.size() * sizeof(MaterialInfo),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " material info buffer",
        command_buffer,
        temp_buffers
    );

    return {
        .positions = std::move(position_buffer),
        .indices = std::move(index_buffer),
        .normals = std::move(normal_buffer),
        .material_ids = std::move(material_id_buffer),
        .uvs = std::move(uv_buffer),
        .material_info = std::move(material_info_buffer),
        .images = std::move(images),
        .image_indices = image_indices,
        .num_indices = static_cast<uint32_t>(indices.size()),
        .bounding_sphere_radius = bounding_sphere_radius,
        .bounding_box = bounding_box};
}

MeshInfo Mesh::get_info(const vk::raii::Device& device) {
    return {
        .positions = device.getBufferAddress({.buffer = positions.buffer}),
        .indices = device.getBufferAddress({.buffer = indices.buffer}),
        .normals = device.getBufferAddress({.buffer = normals.buffer}),
        .uvs = device.getBufferAddress({.buffer = uvs.buffer}),
        .material_indices =
            device.getBufferAddress({.buffer = material_ids.buffer}),
        .material_info =
            device.getBufferAddress({.buffer = material_info.buffer}),
        .num_indices = num_indices,
        .bounding_sphere_radius = bounding_sphere_radius};
}

GltfMesh load_gltf(
    const std::filesystem::path& filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers
) {
    fastgltf::Parser parser(fastgltf::Extensions::KHR_mesh_quantization);
    fastgltf::GltfDataBuffer data;
    data.loadFromFile(filepath);

    auto parent_path = filepath.parent_path();

    auto asset_result =
        parser.loadGLTF(&data, parent_path, fastgltf::Options::None);
    if (auto error = asset_result.error(); error != fastgltf::Error::None) {
        // Some error occurred while reading the buffer, parsing the JSON, or validating the data.
        dbg(fastgltf::getErrorMessage(error));
        abort();
    }

    auto asset = std::move(asset_result.get());

    auto error = fastgltf::validate(asset);
    assert(error == fastgltf::Error::None);

    std::vector<ImageWithView> images;

    for (auto& img : asset.images) {
        std::visit(
            fastgltf::visitor {
                [&](fastgltf::sources::URI& filePath) {
                    auto image_path = parent_path / filePath.uri.fspath();
                    assert(image_path.extension() == ".ktx2");
                    images.push_back(load_ktx2_image(
                        image_path,
                        allocator,
                        device,
                        command_buffer,
                        graphics_queue_family,
                        temp_buffers
                    ));
                },
                [](auto&) { assert(false); },
            },
            img.data
        );
    }

    return {.images = std::move(images)};
}
