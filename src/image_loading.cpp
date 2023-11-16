#include "image_loading.h"

#include "dds.h"
#include "sync.h"

AllocatedImage load_dds(
    const char* filepath,
    vma::Allocator allocator,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers
) {
    std::ifstream stream(filepath, std::ios::binary);

    assert(stream);

    DWORD dwMagic;
    DDS_HEADER header;
    DDS_HEADER_DXT10 header10;
    stream.read((char*)&dwMagic, sizeof dwMagic);

    //auto expected_magic = DWORD{'D', 'D', 'S', ' '};

    //assert(dwMagic == expected_magic);

    stream.read((char*)&header, sizeof header);

    stream.read((char*)&header10, sizeof header10);

    auto format = translate_dxgi_to_vulkan(header10.dxgiFormat);

    auto offset = sizeof dwMagic + sizeof header + sizeof header10;

    assert(header10.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE3D);

    auto width = header.dwWidth;
    auto height = header.dwHeight;
    auto depth = header.dwDepth;

    auto bytes_per_pixel = 4;

    std::cout << offset << " " << vk::to_string(format) << " " << header.dwWidth
              << " " << header.dwHeight << " " << header10.resourceDimension
              << std::endl;

    auto extent = vk::Extent3D {
        .width = width,
        .height = height,
        .depth = depth,
    };

    auto image_name = std::string(filepath) + " image";

    auto image = AllocatedImage(
        vk::ImageCreateInfo {
            .imageType = vk::ImageType::e3D,
            .format = format,
            .extent = extent,
            .mipLevels = 1,
            .arrayLayers = 1,
            .usage = vk::ImageUsageFlagBits::eSampled
                | vk::ImageUsageFlagBits::eTransferDst},
        allocator,
        image_name.data()
    );

    auto data_size = width * height * depth * bytes_per_pixel;

    auto staging_buffer_name = std::string(filepath) + " staging buffer";

    auto staging_buffer = AllocatedBuffer(
        vk::BufferCreateInfo {
            .size = data_size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc},
        {.flags = vma::AllocationCreateFlagBits::eMapped,
         .requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible},
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
            .image = image.image}}
    );

    command_buffer.copyBufferToImage(
        staging_buffer.buffer,
        image.image,
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
            .image = image.image}}
    );

    temp_buffers.push_back(std::move(staging_buffer));

    return image;
}
