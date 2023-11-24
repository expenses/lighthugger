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

    auto mesh_info = MeshInfo {
        .positions =
            device.getBufferAddress({.buffer = position_buffer.buffer}),
        .indices = device.getBufferAddress({.buffer = index_buffer.buffer}),
        .normals = device.getBufferAddress({.buffer = normal_buffer.buffer}),
        .uvs = device.getBufferAddress({.buffer = uv_buffer.buffer}),
        .material_info =
            device.getBufferAddress({.buffer = material_info_buffer.buffer}),
        .material_indices =
            device.getBufferAddress({.buffer = material_id_buffer.buffer}),
        .num_indices = static_cast<uint32_t>(indices.size()),
        .type = TYPE_NORMAL,
        .bounding_sphere_radius = bounding_sphere_radius,
    };

    auto mesh_info_buffer = upload_via_staging_buffer(
        &mesh_info,
        sizeof(MeshInfo),
        allocator,
        vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        std::string(filepath) + " mesh info buffer",
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
        .mesh_info = std::move(mesh_info_buffer),
        .images = std::move(images),
        .image_indices = image_indices};
}

struct NodeTreeNode {
    glm::mat4 transform = glm::mat4(1);
    size_t parent = std::numeric_limits<size_t>::max();
};

struct NodeTree {
    std::vector<NodeTreeNode> inner;

    NodeTree(const fastgltf::Asset& asset) : inner(asset.nodes.size()) {
        for (size_t i = 0; i < asset.nodes.size(); i++) {
            auto& gltf_node = asset.nodes[i];

            if (auto* trs =
                    std::get_if<fastgltf::Node::TRS>(&gltf_node.transform)) {
                // todo: these are probably not correct.
                inner[i].transform = glm::translate(
                    glm::scale(
                        glm::mat4(1.0),
                        glm::vec3(trs->scale[0], trs->scale[1], trs->scale[2])
                    ),
                    glm::vec3(
                        trs->translation[0],
                        trs->translation[1],
                        trs->translation[2]
                    )
                );
            }
        }
    }

    glm::mat4 transform_of(size_t index) {
        glm::mat4 transform_sum = glm::mat4(1.0);

        while (index != std::numeric_limits<size_t>::max()) {
            auto node = inner[index];
            transform_sum = node.transform * transform_sum;
            index = node.parent;
        }

        return transform_sum;
    }
};

void copy_buffer_to_final(
    const fastgltf::Accessor& accessor,
    const fastgltf::Asset& asset,
    const std::vector<PersistentlyMappedBuffer>& staging_buffers,
    const AllocatedBuffer& final_buffer,
    const vk::raii::CommandBuffer& command_buffer
) {
    auto& buffer_view = asset.bufferViews[accessor.bufferViewIndex.value()];
    command_buffer.copyBuffer(
        staging_buffers[buffer_view.bufferIndex].buffer.buffer,
        final_buffer.buffer,
        {vk::BufferCopy {
            .srcOffset = buffer_view.byteOffset,
            .dstOffset = 0,
            .size = buffer_view.byteLength}}
    );
}

void calc_bounding_sphere(
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    const AllocatedBuffer& mesh_info_buffer,
    const Pipelines& pipelines,
    std::vector<DescriptorPoolAndSet>& temp_descriptor_sets,
    uint32_t num_vertices
) {
    auto pool_sizes = std::array {vk::DescriptorPoolSize {
        .type = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1}};

    auto descriptor_pool = device.createDescriptorPool(
        {.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,

         .maxSets = 1,
         .poolSizeCount = pool_sizes.size(),
         .pPoolSizes = pool_sizes.data()}
    );

    auto descriptor_sets_to_create =
        std::array {*pipelines.dsl.calc_bounding_sphere};

    std::vector<vk::raii::DescriptorSet> descriptor_sets =
        device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {
            .descriptorPool = *descriptor_pool,
            .descriptorSetCount =
                static_cast<uint32_t>(descriptor_sets_to_create.size()),
            .pSetLayouts = descriptor_sets_to_create.data()});

    auto descriptor_set = std::move(descriptor_sets[0]);

    auto info = buffer_info(mesh_info_buffer);

    device.updateDescriptorSets(
        {vk::WriteDescriptorSet {
            .dstSet = *descriptor_set,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &info}},
        {}
    );

    command_buffer.bindPipeline(
        vk::PipelineBindPoint::eCompute,
        *pipelines.calc_bounding_sphere.pipeline
    );
    command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        *pipelines.calc_bounding_sphere.layout,
        0,
        {*descriptor_set},
        {}
    );
    command_buffer.dispatch(dispatch_size(num_vertices, 64), 1, 1);

    temp_descriptor_sets.push_back(DescriptorPoolAndSet {
        .pool = std::move(descriptor_pool),
        .set = std::move(descriptor_set)});
}

void copy_uint16_t4_to_uint16_3(
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    const AllocatedBuffer& copy_dst,
    const AllocatedBuffer& copy_src,
    const Pipelines& pipelines,
    uint32_t count,
    uint32_t src_offset,
    bool use_16bit
) {
    command_buffer.bindPipeline(
        vk::PipelineBindPoint::eCompute,
        *pipelines.copy_quantized_positions.pipeline
    );
    command_buffer.pushConstants<CopyQuantizedPositionsConstant>(
        *pipelines.copy_quantized_positions.layout,
        vk::ShaderStageFlagBits::eCompute,
        0,
        {{.dst = device.getBufferAddress({.buffer = copy_dst.buffer}),
          .src =
              device.getBufferAddress({.buffer = copy_src.buffer}) + src_offset,

          .count = count,
          .use_16bit = use_16bit}}
    );
    command_buffer.dispatch(dispatch_size(count, 64), 1, 1);
}

GltfMesh load_gltf(
    const std::filesystem::path& filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers,
    DescriptorSet& descriptor_set,
    const Pipelines& pipelines,
    std::vector<DescriptorPoolAndSet>& temp_descriptor_sets
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

    std::vector<PersistentlyMappedBuffer> staging_buffers;
    staging_buffers.reserve(asset.buffers.size());
    for (auto& buffer : asset.buffers) {
        if (auto* uri = std::get_if<fastgltf::sources::URI>(&buffer.data)) {
            std::ifstream stream(
                parent_path / uri->uri.fspath(),
                std::ios::binary
            );
            auto staging_buffer = PersistentlyMappedBuffer(AllocatedBuffer(
                vk::BufferCreateInfo {
                    .size = buffer.byteLength,
                    .usage = vk::BufferUsageFlagBits::eTransferSrc
                        | vk::BufferUsageFlagBits::eShaderDeviceAddress},
                {
                    .flags = vma::AllocationCreateFlagBits::eMapped
                        | vma::AllocationCreateFlagBits::
                            eHostAccessSequentialWrite,
                    .usage = vma::MemoryUsage::eAuto,
                },
                allocator,
                uri->uri.fspath()
            ));
            stream.read(
                reinterpret_cast<char*>(staging_buffer.mapped_ptr),
                static_cast<std::streamsize>(buffer.byteLength)
            );
            staging_buffers.push_back(std::move(staging_buffer));
        } else {
            dbg("here");
            abort();
        }
    }

    std::vector<ImageWithView> images;
    std::vector<uint32_t> image_indices;
    images.reserve(asset.images.size());
    image_indices.reserve(asset.images.size());

    for (auto& img : asset.images) {
        if (auto* uri = std::get_if<fastgltf::sources::URI>(&img.data)) {
            auto image_path = parent_path / uri->uri.fspath();
            assert(image_path.extension() == ".ktx2");
            auto image = load_ktx2_image(
                image_path,
                allocator,
                device,
                command_buffer,
                graphics_queue_family,
                temp_buffers
            );
            auto index = descriptor_set.write_image(image, *device);
            images.push_back(std::move(image));
            image_indices.push_back(index);
        }
    }

    std::vector<MaterialInfo> material_info;
    material_info.reserve(asset.materials.size());

    for (auto& material : asset.materials) {
        material_info.push_back(
            {.albedo_texture_index =
                 image_indices[material.pbrData.baseColorTexture.value()
                                   .textureIndex],
             .albedo_texture_scale = glm::vec2(0.000242184135, 0.000240402936),
             .albedo_texture_offset = glm::vec2(0.0036249999, 0.00644397736)}
        );
    }

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

    auto node_tree = NodeTree(asset);

    std::vector<GltfPrimitive> primitives;

    for (size_t i = 0; i < asset.nodes.size(); i++) {
        auto& node = asset.nodes[i];

        if (node.meshIndex) {
            auto transform = node_tree.transform_of(i);

            auto& mesh = asset.meshes[node.meshIndex.value()];

            for (auto& primitive : mesh.primitives) {
                auto create_buffer = [&](uint64_t size, const char* name) {
                    return AllocatedBuffer(
                        vk::BufferCreateInfo {
                            .size = size,
                            .usage = vk::BufferUsageFlagBits::eTransferDst
                                | vk::BufferUsageFlagBits::eStorageBuffer
                                | vk::BufferUsageFlagBits::
                                    eShaderDeviceAddress},
                        {
                            .usage = vma::MemoryUsage::eAuto,
                        },
                        allocator,
                        std::string(filepath) + " " + name
                    );
                };

                auto material_index = primitive.materialIndex.value();

                auto position_it = primitive.findAttribute("POSITION");
                assert(position_it != primitive.attributes.end());
                auto& position_accessor = asset.accessors[position_it->second];

                assert(
                    position_accessor.componentType
                    == fastgltf::ComponentType::UnsignedShort
                );
                assert(position_accessor.type == fastgltf::AccessorType::Vec3);
                auto position_buffer = create_buffer(
                    position_accessor.count * sizeof(uint16_t) * 3,
                    "positions"
                );
                {
                    auto& buffer_view =
                        asset.bufferViews[position_accessor.bufferViewIndex
                                              .value()];

                    copy_uint16_t4_to_uint16_3(
                        device,
                        command_buffer,
                        position_buffer,
                        staging_buffers[buffer_view.bufferIndex].buffer,
                        pipelines,
                        position_accessor.count,
                        buffer_view.byteOffset,
                        true
                    );
                }

                auto& indices =
                    asset.accessors[primitive.indicesAccessor.value()];
                assert(
                    indices.componentType
                    == fastgltf::ComponentType::UnsignedInt
                );
                assert(indices.type == fastgltf::AccessorType::Scalar);
                auto indices_buffer =
                    create_buffer(indices.count * sizeof(uint32_t), "indices");
                copy_buffer_to_final(
                    indices,
                    asset,
                    staging_buffers,
                    indices_buffer,
                    command_buffer
                );

                auto uvs_it = primitive.findAttribute("TEXCOORD_0");
                assert(uvs_it != primitive.attributes.end());
                auto& uvs = asset.accessors[uvs_it->second];
                assert(
                    uvs.componentType == fastgltf::ComponentType::UnsignedShort
                );
                assert(uvs.type == fastgltf::AccessorType::Vec2);
                auto uvs_buffer =
                    create_buffer(uvs.count * sizeof(uint16_t) * 2, "uvs");
                copy_buffer_to_final(
                    uvs,
                    asset,
                    staging_buffers,
                    uvs_buffer,
                    command_buffer
                );

                auto normals_it = primitive.findAttribute("NORMAL");
                assert(normals_it != primitive.attributes.end());
                auto& normals = asset.accessors[normals_it->second];
                assert(normals.componentType == fastgltf::ComponentType::Byte);
                assert(normals.type == fastgltf::AccessorType::Vec3);
                auto normals_buffer = create_buffer(
                    normals.count * sizeof(int8_t) * 4,
                    "normals"
                );
                copy_buffer_to_final(
                    normals,
                    asset,
                    staging_buffers,
                    normals_buffer,
                    command_buffer
                );
                /*
                // See copy_quantized_positions.hlsl.
                {
                    auto& buffer_view =
                        asset.bufferViews[normals.bufferViewIndex
                                              .value()];

                    copy_uint16_t4_to_uint16_3(
                        device,
                        command_buffer,
                        normals_buffer,
                        staging_buffers[buffer_view.bufferIndex].buffer,
                        pipelines,
                        normals.count,
                        buffer_view.byteOffset,
                        false
                    );
                }*/

                auto mesh_info = MeshInfo {
                    .positions = device.getBufferAddress(
                        {.buffer = position_buffer.buffer}
                    ),
                    .indices = device.getBufferAddress(
                        {.buffer = indices_buffer.buffer}
                    ),
                    .normals = device.getBufferAddress(
                        {.buffer = normals_buffer.buffer}
                    ),
                    .uvs =
                        device.getBufferAddress({.buffer = uvs_buffer.buffer}),
                    .material_info = device.getBufferAddress(
                        {.buffer = material_info_buffer.buffer}
                    ),
                    .material_indices = material_index,
                    .num_indices = static_cast<uint32_t>(indices.count),
                    .type = TYPE_QUANITZED,
                    .bounding_sphere_radius = 0.0f};

                auto mesh_info_buffer = upload_via_staging_buffer(
                    &mesh_info,
                    sizeof(MeshInfo),
                    allocator,
                    vk::BufferUsageFlagBits::eStorageBuffer
                        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                    std::string(filepath) + " mesh info buffer",
                    command_buffer,
                    temp_buffers
                );

                calc_bounding_sphere(
                    device,
                    command_buffer,
                    mesh_info_buffer,
                    pipelines,
                    temp_descriptor_sets,
                    position_accessor.count
                );

                primitives.push_back(GltfPrimitive {
                    .position = std::move(position_buffer),
                    .indices = std::move(indices_buffer),
                    .uvs = std::move(uvs_buffer),
                    .normals = std::move(normals_buffer),
                    .mesh_info = std::move(mesh_info_buffer),
                    .transform = transform});
            }
        }
    }

    for (auto& staging_buffer : staging_buffers) {
        temp_buffers.push_back(std::move(staging_buffer.buffer));
    }

    return {
        .images = std::move(images),
        .material_info = std::move(material_info_buffer),
        .primitives = std::move(primitives)};
}
