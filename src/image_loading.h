#include "dds.h"

const std::array<uint8_t, 12> KTX2_IDENTIFIER = {
    0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
};

struct Ktx2Header {
    vk::Format format;
    uint32_t type_size;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t layer_count;
    uint32_t face_count;
    uint32_t level_count;
    uint32_t supercompression_scheme;
};

struct Ktx2Index {
    uint32_t dfd_byte_offset;
    uint32_t dfd_byte_length;
    uint32_t kvd_byte_offset;
    uint32_t kvd_byte_length;
    uint64_t sgd_byte_offset;
    uint64_t sgd_byte_length;
};

struct Ktx2LevelIndex {
    uint64_t byte_offset;
    uint64_t byte_length;
    uint64_t uncompressed_byte_length;
};

void load_ktx2_image(const char* filepath) {
    std::ifstream stream(filepath, std::ios::binary);

    std::array<uint8_t, 12> identifier;
    stream.read((char*)identifier.data(), identifier.size());

    assert(identifier == KTX2_IDENTIFIER);

    Ktx2Header header;

    stream.read((char*) &header, sizeof header);

    std::cout << vk::to_string(header.format) << " " << header.supercompression_scheme << std::endl;

    Ktx2Index index;

    stream.read((char*) &index, sizeof index);

    std::vector<Ktx2LevelIndex> levels(std::max(1u, header.level_count));

    for (int i = 0; i < std::max(1u, header.level_count); i++) {
        stream.read((char*) &levels[i], sizeof levels[i]);
    }

    std::cout << levels[0].byte_length << " " << levels[0].uncompressed_byte_length << std::endl;

    // Only uncompressed images are handled for now.
    assert(header.supercompression_scheme == 0);
}

struct LoadedImage {
    AllocatedImage image;
    AllocatedBuffer staging_buffer;
};

LoadedImage load_dds(const char* filepath, vma::Allocator allocator, const vk::raii::CommandBuffer& command_buffer, uint32_t graphics_queue_family) {
    std::ifstream stream(filepath, std::ios::binary);

    DWORD dwMagic;
    DDS_HEADER header;
    DDS_HEADER_DXT10 header10;
    stream.read((char*) &dwMagic, sizeof dwMagic);

    //auto expected_magic = DWORD{'D', 'D', 'S', ' '};

    //assert(dwMagic == expected_magic);

    stream.read((char*) &header, sizeof header);

    stream.read((char*) &header10, sizeof header10);

    auto format = translate_dxgi_to_vulkan(header10.dxgiFormat);

    auto offset = sizeof dwMagic + sizeof header + sizeof header10;

    assert(header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D);

    auto width = header.dwWidth;
    auto height = header.dwHeight;
    auto depth = header.dwDepth;

    auto bpp = 4;

    std::cout << offset << " " << vk::to_string(format) << " " << header.dwWidth << " " << header.dwHeight << " " << header10.resourceDimension << std::endl;

    auto extent = vk::Extent3D {
        .width = width,
        .height = height,
        .depth = depth,
    };

    auto image_name = std::string(filepath) + " image";

    auto image = AllocatedImage(vk::ImageCreateInfo {
        .imageType = vk::ImageType::e3D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
    }, allocator, image_name.data());

    auto data_size = width * height * depth * 4;

    auto staging_buffer_name = std::string(filepath) + " staging buffer";

    auto staging_buffer = AllocatedBuffer(vk::BufferCreateInfo{
        .size = data_size,
        .usage = vk::BufferUsageFlagBits::eTransferSrc
    }, {.flags = vma::AllocationCreateFlagBits::eMapped, .requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible }, allocator);

    auto info = allocator.getAllocationInfo(staging_buffer.allocation);

    assert(info.pMappedData);

    stream.read((char*) info.pMappedData, data_size);

    insert_color_image_barrier(command_buffer, {
        .prev_access = THSVS_ACCESS_NONE,
        .next_access = THSVS_ACCESS_TRANSFER_WRITE,
        .discard_contents = true,
        .queue_family = graphics_queue_family,
        .image = image.image
    });

    command_buffer.copyBufferToImage(staging_buffer.buffer, image.image, vk::ImageLayout::eTransferDstOptimal, {
        vk::BufferImageCopy {
            .bufferOffset = 0,
            .bufferRowLength = width,
            .bufferImageHeight = height,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {},
            .imageExtent = extent
        }
    });

    insert_color_image_barrier(command_buffer, {
        .prev_access = THSVS_ACCESS_TRANSFER_WRITE,
        .next_access = THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
        .discard_contents = false,
        .queue_family = graphics_queue_family,
        .image = image.image
    });

    return {
        .image = std::move(image),
        .staging_buffer = std::move(staging_buffer)
    };
}
