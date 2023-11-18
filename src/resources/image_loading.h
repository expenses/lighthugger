#include "../allocations.h"
#include "../util.h"

ImageWithView load_dds(
    const char* filepath,
    vma::Allocator allocator,
    const vk::raii::Device& device,
    const vk::raii::CommandBuffer& command_buffer,
    uint32_t graphics_queue_family,
    std::vector<AllocatedBuffer>& temp_buffers
);

/*
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
*/
