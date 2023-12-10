#pragma once

struct AllocatedImage {
    vk::Image image;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedImage(AllocatedImage&& other);

    AllocatedImage(
        vk::ImageCreateInfo create_info,
        vma::Allocator allocator_,
        const std::string& name
    );

    AllocatedImage& operator=(AllocatedImage&& other);

    ~AllocatedImage();
};

struct AllocatedBuffer {
    vk::Buffer buffer;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedBuffer(AllocatedBuffer&& other);

    AllocatedBuffer(
        vk::BufferCreateInfo create_info,
        vma::AllocationCreateInfo alloc_info,
        vma::Allocator allocator_,
        const std::string& name
    );

    ~AllocatedBuffer();

    AllocatedBuffer& operator=(AllocatedBuffer&& other);
};
