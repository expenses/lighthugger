#include "mesh_loading.h"

#include "../allocations/staging.h"
#include "image_loading.h"

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
    const vk::raii::CommandBuffer& command_buffer,
    const uint32_t buffer_size
) {
    auto& buffer_view = asset.bufferViews[accessor.bufferViewIndex.value()];
    command_buffer.copyBuffer(
        staging_buffers[buffer_view.bufferIndex].buffer.buffer,
        final_buffer.buffer,
        {vk::BufferCopy {
            .srcOffset = buffer_view.byteOffset + accessor.byteOffset,
            .dstOffset = 0,
            .size = buffer_size}}
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
    command_buffer.pushConstants<CalcBoundingSphereConstant>(
        *pipelines.calc_bounding_sphere.layout,
        vk::ShaderStageFlagBits::eCompute,
        0,
        {{.num_vertices = num_vertices}}
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
    uint32_t src_offset
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

          .count = count}}
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
    fastgltf::Parser parser(
        fastgltf::Extensions::KHR_mesh_quantization
        | fastgltf::Extensions::KHR_texture_transform
    );
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

    std::vector<std::vector<uint8_t>> source_buffers;
    std::vector<PersistentlyMappedBuffer> staging_buffers;
    staging_buffers.reserve(asset.buffers.size());
    source_buffers.reserve(asset.buffers.size());
    for (auto& buffer : asset.buffers) {
        if (auto* uri = std::get_if<fastgltf::sources::URI>(&buffer.data)) {
            auto source_buffer =
                read_file_to_bytes(parent_path / uri->uri.fspath());

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

            memcpy(
                staging_buffer.mapped_ptr,
                source_buffer.data(),
                buffer.byteLength
            );
            staging_buffers.push_back(std::move(staging_buffer));
            source_buffers.push_back(std::move(source_buffer));
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

    auto node_tree = NodeTree(asset);

    std::vector<GltfPrimitive> primitives;

    for (size_t i = 0; i < asset.nodes.size(); i++) {
        auto& node = asset.nodes[i];

        if (node.meshIndex) {
            auto transform = node_tree.transform_of(i);

            auto& mesh = asset.meshes[node.meshIndex.value()];

            for (size_t i = 0; i < mesh.primitives.size(); i++) {
                auto& primitive = mesh.primitives[i];

                auto primitive_name =
                    std::string(filepath) + " primitive " + std::to_string(i);

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
                        primitive_name + " " + name
                    );
                };

                auto get_accessor = [&](const char* name
                                    ) -> fastgltf::Accessor& {
                    auto iterator = primitive.findAttribute(name);
                    assert(iterator != primitive.attributes.end());
                    return asset.accessors[iterator->second];
                };

                auto& positions = get_accessor("POSITION");
                assert(
                    positions.componentType
                    == fastgltf::ComponentType::UnsignedShort
                );
                assert(positions.type == fastgltf::AccessorType::Vec3);
                auto position_buffer = create_buffer(
                    positions.count * sizeof(uint16_t) * 3,
                    "positions"
                );

                auto& positions_buffer_view =
                    asset.bufferViews[positions.bufferViewIndex.value()];

                {
                    copy_uint16_t4_to_uint16_3(
                        device,
                        command_buffer,
                        position_buffer,
                        staging_buffers[positions_buffer_view.bufferIndex]
                            .buffer,
                        pipelines,
                        positions.count,
                        positions_buffer_view.byteOffset + positions.byteOffset
                    );
                }

                auto& indices =
                    asset.accessors[primitive.indicesAccessor.value()];

                bool uses_32_bit_indices = indices.componentType
                    == fastgltf::ComponentType::UnsignedInt;
                if (!uses_32_bit_indices) {
                    assert(
                        indices.componentType
                        == fastgltf::ComponentType::UnsignedShort
                    );
                }
                assert(indices.type == fastgltf::AccessorType::Scalar);

                auto& uvs = get_accessor("TEXCOORD_0");
                assert(
                    uvs.componentType == fastgltf::ComponentType::UnsignedShort
                );
                assert(uvs.type == fastgltf::AccessorType::Vec2);
                auto uvs_buffer_size = uvs.count * sizeof(uint16_t) * 2;
                auto uvs_buffer =
                    create_buffer(uvs.count * sizeof(uint16_t) * 2, "uvs");
                copy_buffer_to_final(
                    uvs,
                    asset,
                    staging_buffers,
                    uvs_buffer,
                    command_buffer,
                    uvs_buffer_size
                );

                auto& normals = get_accessor("NORMAL");
                assert(normals.componentType == fastgltf::ComponentType::Byte);
                assert(normals.type == fastgltf::AccessorType::Vec3);
                auto normals_buffer_size = normals.count * sizeof(int8_t) * 4;
                auto normals_buffer = create_buffer(
                    normals.count * sizeof(int8_t) * 4,
                    "normals"
                );
                copy_buffer_to_final(
                    normals,
                    asset,
                    staging_buffers,
                    normals_buffer,
                    command_buffer,
                    normals_buffer_size
                );

                auto material_index = primitive.materialIndex.value();
                auto& material = asset.materials[material_index];

                auto texture_scale = glm::vec2(1.0);
                auto texture_offset = glm::vec2(0.0);
                auto albedo_texture_index =
                    std::numeric_limits<uint32_t>::max();
                auto metallic_roughness_texture_index =
                    std::numeric_limits<uint32_t>::max();
                auto normal_texture_index =
                    std::numeric_limits<uint32_t>::max();

                if (material.pbrData.baseColorTexture) {
                    auto& tex = material.pbrData.baseColorTexture.value();

                    albedo_texture_index =
                        image_indices[asset.textures[tex.textureIndex]
                                          .imageIndex.value()];

                    if (tex.transform != nullptr) {
                        texture_scale = glm::vec2(
                            (*tex.transform).uvScale[0],
                            (*tex.transform).uvScale[1]
                        );
                        texture_offset = glm::vec2(
                            (*tex.transform).uvOffset[0],
                            (*tex.transform).uvOffset[1]
                        );
                    }
                }

                if (material.pbrData.metallicRoughnessTexture) {
                    auto& tex =
                        material.pbrData.metallicRoughnessTexture.value();
                    metallic_roughness_texture_index =
                        image_indices[asset.textures[tex.textureIndex]
                                          .imageIndex.value()];

                    if (tex.transform != nullptr) {
                        texture_scale = glm::vec2(
                            (*tex.transform).uvScale[0],
                            (*tex.transform).uvScale[1]
                        );
                        texture_offset = glm::vec2(
                            (*tex.transform).uvOffset[0],
                            (*tex.transform).uvOffset[1]
                        );
                    }
                }

                if (material.normalTexture) {
                    auto& tex = material.normalTexture.value();
                    normal_texture_index =
                        image_indices[asset.textures[tex.textureIndex]
                                          .imageIndex.value()];

                    if (tex.transform != nullptr) {
                        texture_scale = glm::vec2(
                            (*tex.transform).uvScale[0],
                            (*tex.transform).uvScale[1]
                        );
                        texture_offset = glm::vec2(
                            (*tex.transform).uvOffset[0],
                            (*tex.transform).uvOffset[1]
                        );
                    }
                }

                if (uvs.normalized) {
                    texture_scale /= float((1 << 16) - 1);
                }

                uint32_t flags = 0;
                if (uses_32_bit_indices) {
                    flags |= MESH_INFO_FLAGS_32_BIT_INDICES;
                }
                if (material.alphaMode == fastgltf::AlphaMode::Mask) {
                    flags |= MESH_INFO_FLAGS_ALPHA_CLIP;
                }

                if (material.alphaCutoff != 0.5) {
                    dbg(material.alphaCutoff);
                    abort();
                }

                // Assume that all alpha clipped geometry is double-sided
                // and that all opaque geometry is single sided.
                if ((material.alphaMode == fastgltf::AlphaMode::Mask)
                    != material.doubleSided) {
                    dbg((material.alphaMode == fastgltf::AlphaMode::Mask),
                        material.doubleSided);
                    abort();
                }

                assert(material.pbrData.baseColorFactor[3] == 1.0);

                // todo: albedo factor, proper pbr.

                std::vector<float> float_positions(positions.count * 3);

                uint16_t* uint_positions = reinterpret_cast<uint16_t*>(
                    source_buffers[positions_buffer_view.bufferIndex].data()
                    + positions_buffer_view.byteOffset + positions.byteOffset
                );
                for (size_t i = 0; i < positions.count; i++) {
                    assert(uint_positions[i * 4 + 3] == 0);
                    float_positions[i * 3] = float(uint_positions[i * 4]);
                    float_positions[i * 3 + 1] =
                        float(uint_positions[i * 4 + 1]);
                    float_positions[i * 3 + 2] =
                        float(uint_positions[i * 4 + 2]);
                }

                auto& indices_buffer_view =
                    asset.bufferViews[indices.bufferViewIndex.value()];

                auto meshlets = build_meshlets(
                    source_buffers[indices_buffer_view.bufferIndex].data()
                        + indices_buffer_view.byteOffset + indices.byteOffset,
                    indices.count,
                    float_positions.data(),
                    positions.count,
                    uses_32_bit_indices
                );

                std::optional<AllocatedBuffer> indices_buffer = std::nullopt;

                if (uses_32_bit_indices) {
                    indices_buffer = upload_via_staging_buffer(
                        meshlets.indices_32bit.data(),
                        meshlets.indices_32bit.size() * sizeof(uint32_t),
                        allocator,
                        vk::BufferUsageFlagBits::eStorageBuffer
                            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                        primitive_name + " indices buffer",
                        command_buffer,
                        temp_buffers
                    );
                } else {
                    indices_buffer = upload_via_staging_buffer(
                        meshlets.indices_16bit.data(),
                        meshlets.indices_16bit.size() * sizeof(uint16_t),
                        allocator,
                        vk::BufferUsageFlagBits::eStorageBuffer
                            | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                        primitive_name + " indices buffer",
                        command_buffer,
                        temp_buffers
                    );
                }

                auto micro_indices = upload_via_staging_buffer(
                    meshlets.micro_indices.data(),
                    meshlets.micro_indices.size(),
                    allocator,
                    vk::BufferUsageFlagBits::eStorageBuffer
                        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                    primitive_name + " micro indices buffer",
                    command_buffer,
                    temp_buffers
                );

                auto meshlets_buffer = upload_via_staging_buffer(
                    meshlets.meshlets.data(),
                    meshlets.meshlets.size() * sizeof(Meshlet),
                    allocator,
                    vk::BufferUsageFlagBits::eStorageBuffer
                        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                    primitive_name + " meshlets buffer",
                    command_buffer,
                    temp_buffers
                );

                auto mesh_info = MeshInfo {
                    .positions = device.getBufferAddress(
                        {.buffer = position_buffer.buffer}
                    ),
                    .indices = device.getBufferAddress(
                        {.buffer = indices_buffer.value().buffer}
                    ),
                    .normals = device.getBufferAddress(
                        {.buffer = normals_buffer.buffer}
                    ),
                    .uvs =
                        device.getBufferAddress({.buffer = uvs_buffer.buffer}),
                    .micro_indices =
                        device.getBufferAddress({.buffer = micro_indices.buffer}
                        ),
                    .meshlets = device.getBufferAddress(
                        {.buffer = meshlets_buffer.buffer}
                    ),
                    .num_meshlets = meshlets.meshlets.size(),

                    .num_indices = static_cast<uint32_t>(indices.count),
                    .flags = flags,
                    .bounding_sphere_radius = 0.0f,
                    .texture_scale = texture_scale,
                    .texture_offset = texture_offset,
                    .albedo_texture_index = albedo_texture_index,
                    .metallic_roughness_texture_index =
                        metallic_roughness_texture_index,
                    .normal_texture_index = normal_texture_index,
                    .albedo_factor = glm::vec3(
                        material.pbrData.baseColorFactor[0],
                        material.pbrData.baseColorFactor[1],
                        material.pbrData.baseColorFactor[2]
                    )};

                auto mesh_info_buffer = upload_via_staging_buffer(
                    &mesh_info,
                    sizeof(MeshInfo),
                    allocator,
                    vk::BufferUsageFlagBits::eStorageBuffer
                        | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                    primitive_name + " mesh info buffer",
                    command_buffer,
                    temp_buffers
                );

                auto triangle_count = indices.count / 3;
                if (triangle_count >= (1 << 16)) {
                    dbg(triangle_count);
                    abort();
                }

                calc_bounding_sphere(
                    device,
                    command_buffer,
                    mesh_info_buffer,
                    pipelines,
                    temp_descriptor_sets,
                    positions.count
                );

                primitives.push_back(GltfPrimitive {
                    .position = std::move(position_buffer),
                    .indices = std::move(indices_buffer.value()),
                    .uvs = std::move(uvs_buffer),
                    .normals = std::move(normals_buffer),
                    .mesh_info = std::move(mesh_info_buffer),
                    .micro_indices = std::move(micro_indices),
                    .meshlets = std::move(meshlets_buffer),
                    .transform = transform});
            }
        }
    }

    for (auto& staging_buffer : staging_buffers) {
        temp_buffers.push_back(std::move(staging_buffer.buffer));
    }

    return {
        .images = std::move(images),
        .image_indices = std::move(image_indices),
        .primitives = std::move(primitives)};
}
