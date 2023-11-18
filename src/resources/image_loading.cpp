#include "image_loading.h"

#include "dds.h"
#include "../sync.h"

vk::Format translate_dxgi_to_vulkan(DXGI_FORMAT dxgi_format) {
    switch (dxgi_format) {
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
            return vk::Format::eE5B9G9R9UfloatPack32;
        case DXGI_FORMAT_BC1_UNORM:
            return vk::Format::eBc1RgbSrgbBlock;
        default:
            dbg(dxgi_format);
            assert(false);
    }
}

vk::ImageType translate_resource_dimension(D3D10_RESOURCE_DIMENSION dimension) {
    switch (dimension) {
        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            return vk::ImageType::e3D;
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            return vk::ImageType::e2D;
        default:
            dbg(dimension);
            assert(false);
    }
}

vk::ImageViewType
resource_dimension_to_view_type(D3D10_RESOURCE_DIMENSION dimension) {
    switch (dimension) {
        case D3D10_RESOURCE_DIMENSION_TEXTURE3D:
            return vk::ImageViewType::e3D;
        case D3D10_RESOURCE_DIMENSION_TEXTURE2D:
            return vk::ImageViewType::e2D;
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

    auto format = translate_dxgi_to_vulkan(header10.dxgiFormat);

    auto data_offset = sizeof dwMagic + sizeof header + sizeof header10;

    auto image_type = translate_resource_dimension(header10.resourceDimension);
    auto image_view_type =
        resource_dimension_to_view_type(header10.resourceDimension);

    auto width = header.dwWidth;
    auto height = header.dwHeight;
    auto depth = std::max(header.dwDepth, 1u);

    // Todo: actually implement this properly based on image format.
    auto bits_per_pixel = depth > 1 ? 32 : 4;

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
            .imageType = image_type,
            .format = format,
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
        image_view_type
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
        buffer_offset +=
            (round_up(level_width, 4) * round_up(level_height, 4) * depth)
            * bits_per_pixel / 8;
    }

    // Make sure we use every byte of the dds file, because otherwise we're probably doing something wrong.
    //dbg(buffer_offset, bytes_remaining, filepath, bits_per_pixel);
    assert(buffer_offset == bytes_remaining);

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