const std::array<uint8_t, 12> KTX2_IDENTIFIER =
    {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};

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
