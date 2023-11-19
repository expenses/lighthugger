#include "image_loading.h"

#include "dds.h"
#include "../sync.h"

struct FormatInfo {
    vk::Format format;
    uint32_t bits_per_pixel;
    bool is_block_compressed = false;
};

FormatInfo translate_format(DXGI_FORMAT dxgi_format) {
    switch (dxgi_format) {
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return {.format = vk::Format::eE5B9G9R9UfloatPack32, .bits_per_pixel = 32};
        case DXGI_FORMAT_BC1_UNORM:
            return {.format = vk::Format::eBc1RgbSrgbBlock, .bits_per_pixel = 4, .is_block_compressed = true};
        default:
            dbg(dxgi_format);
            abort();
    }
}

struct Dimension {
    vk::ImageType type;
    vk::ImageViewType view_type;
};

Dimension translate_dimension(D3D10_RESOURCE_DIMENSION dimension) {
    switch (dimension) {
        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            return {.type = vk::ImageType::e3D, .view_type = vk::ImageViewType::e3D};
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
             return {.type = vk::ImageType::e2D, .view_type = vk::ImageViewType::e2D};
        default:
            dbg(dimension);
            assert(false);
    }
}

uint32_t round_up(uint32_t value, uint32_t round_to) {
    return uint32_t(std::ceil(float(value) / float(round_to))) * round_to;
}

ImageWithView load_dds(
    const char* filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers
) {
    std::ifstream stream(filepath, std::ios::binary);

    assert(stream);

    std::array<char, 4> dwMagic;
    DDS_HEADER header;
    DDS_HEADER_DXT10 header10;
    stream.read(dwMagic.data(), sizeof dwMagic);

    auto expected_magic = std::array {'D', 'D', 'S', ' '};

    assert(dwMagic == expected_magic);

    stream.read((char*)&header, sizeof header);

    stream.read((char*)&header10, sizeof header10);

    auto format = translate_format(header10.dxgiFormat);

    auto data_offset = sizeof dwMagic + sizeof header + sizeof header10;

    auto dimension = translate_dimension(header10.resourceDimension);

    auto width = header.dwWidth;
    auto height = header.dwHeight;
    auto depth = std::max(header.dwDepth, 1u);

    stream.seekg(0, stream.end);
    auto bytes_remaining = uint32_t(stream.tellg()) - data_offset;
    stream.seekg(data_offset, stream.beg);

    auto mip_levels = header.dwMipMapCount;

    auto subresource_range = vk::ImageSubresourceRange {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = mip_levels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    auto image_name = std::string(filepath) + " image";

    auto image = ImageWithView(
        vk::ImageCreateInfo {
            .imageType = dimension.type,
            .format = format.format,
            .extent =
                vk::Extent3D {
                    .width = width,
                    .height = height,
                    .depth = depth,
                },
            .mipLevels = mip_levels,
            .arrayLayers = 1,
            .usage = vk::ImageUsageFlagBits::eSampled
                | vk::ImageUsageFlagBits::eTransferDst},
        allocator,
        device,
        image_name.data(),
        subresource_range,
        dimension.view_type
    );

    auto staging_buffer_name = std::string(filepath) + " staging buffer";

    auto staging_buffer = AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = bytes_remaining,
            .usage = vk::BufferUsageFlagBits::eTransferSrc},
        {
            .flags = vma::AllocationCreateFlagBits::eMapped
                | vma::AllocationCreateFlagBits::eHostAccessSequentialWrite,
            .usage = vma::MemoryUsage::eAuto,
        },
        allocator
    );

    auto info = allocator.getAllocationInfo(staging_buffer.allocation);

    assert(info.pMappedData);

    stream.read((char*)info.pMappedData, bytes_remaining);

    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_NONE,
            .next_access = THSVS_ACCESS_TRANSFER_WRITE,
            .discard_contents = true,
            .queue_family = graphics_queue_family,
            .image = image.image.image,
            .subresource_range = subresource_range}}
    );

    uint64_t buffer_offset = 0;

    std::vector<vk::BufferImageCopy> regions(mip_levels);

    for (uint32_t i = 0; i < mip_levels; i++) {
        auto level_width = std::max(width >> i, 1u);
        auto level_height = std::max(height >> i, 1u);

        regions[i] = vk::BufferImageCopy {
            .bufferOffset = buffer_offset,
            .imageSubresource =
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = uint32_t(i),
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageExtent = vk::Extent3D {
                .width = level_width,
                .height = level_height,
                .depth = depth}};

        // We need to round up the width and heights here because for block
        // compressed textures, the minimum amount of data a miplevel can use
        // is the equivalent of 4x4 pixels, even when the actual mip size is smaller.
        auto rounded_width = format.is_block_compressed ? round_up(level_width, 4) : level_width;
        auto rounded_height = format.is_block_compressed ? round_up(level_height, 4) : level_height;

        buffer_offset +=
            (rounded_width * rounded_height * depth)
            * format.bits_per_pixel / 8;
    }

    if (buffer_offset != bytes_remaining) {
        dbg(buffer_offset, bytes_remaining, filepath, format.bits_per_pixel);
        assert(buffer_offset == bytes_remaining);
    }

    command_buffer.copyBufferToImage(
        staging_buffer.buffer,
        image.image.image,
        vk::ImageLayout::eTransferDstOptimal,
        regions
    );

    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_TRANSFER_WRITE,
            .next_access =
                THSVS_ACCESS_FRAGMENT_SHADER_READ_SAMPLED_IMAGE_OR_UNIFORM_TEXEL_BUFFER,
            .discard_contents = false,
            .queue_family = graphics_queue_family,
            .image = image.image.image}}
    );

    temp_buffers.push_back(std::move(staging_buffer));

    return image;
}
