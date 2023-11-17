#include "image_loading.h"

#include "dds.h"
#include "sync.h"

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

    auto offset = sizeof dwMagic + sizeof header + sizeof header10;

    auto image_type = translate_resource_dimension(header10.resourceDimension);
    auto image_view_type =
        resource_dimension_to_view_type(header10.resourceDimension);

    auto width = header.dwWidth;
    auto height = header.dwHeight;
    auto depth = std::max(header.dwDepth, 1u);

    auto bytes_per_pixel = 4;

    //dbg(offset, vk::to_string(format), header.dwWidth, header.dwHeight, header10.resourceDimension);

    auto extent = vk::Extent3D {
        .width = width,
        .height = height,
        .depth = depth,
    };

    auto image_name = std::string(filepath) + " image";

    auto image = create_image_with_view(
        vk::ImageCreateInfo {
            .imageType = image_type,
            .format = format,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .usage = vk::ImageUsageFlagBits::eSampled
                | vk::ImageUsageFlagBits::eTransferDst},
        allocator,
        device,
        image_name.data(),
        image_view_type
    );

    auto data_size = width * height * depth * bytes_per_pixel;

    auto staging_buffer_name = std::string(filepath) + " staging buffer";

    auto staging_buffer = AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = data_size,
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

    stream.read((char*)info.pMappedData, data_size);

    insert_color_image_barriers(
        command_buffer,
        std::array {ImageBarrier {
            .prev_access = THSVS_ACCESS_NONE,
            .next_access = THSVS_ACCESS_TRANSFER_WRITE,
            .discard_contents = true,
            .queue_family = graphics_queue_family,
            .image = image.image.image}}
    );

    command_buffer.copyBufferToImage(
        staging_buffer.buffer,
        image.image.image,
        vk::ImageLayout::eTransferDstOptimal,
        {vk::BufferImageCopy {
            .bufferOffset = 0,
            .bufferRowLength = width,
            .bufferImageHeight = height,
            .imageSubresource =
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            .imageOffset = {},
            .imageExtent = extent}}
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
