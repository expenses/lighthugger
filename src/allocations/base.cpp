#include "base.h"

#include "../util.h"

AllocatedImage::AllocatedImage(AllocatedImage&& other) {
    std::swap(image, other.image);
    std::swap(allocation, other.allocation);
    std::swap(allocator, other.allocator);
}

AllocatedImage& AllocatedImage::operator=(AllocatedImage&& other) {
    std::swap(image, other.image);
    std::swap(allocation, other.allocation);
    std::swap(allocator, other.allocator);
    return *this;
}

AllocatedImage::~AllocatedImage() {
    if (allocator)
        allocator.destroyImage(image, allocation);
}

AllocatedImage::AllocatedImage(
    vk::ImageCreateInfo create_info,
    vma::Allocator allocator_,
    const std::string& name
) {
    allocator = allocator_;
    vma::AllocationCreateInfo alloc_info = {.usage = vma::MemoryUsage::eAuto};
    check_vk_result(allocator.createImage(
        &create_info,
        &alloc_info,
        &image,
        &allocation,
        nullptr
    ));

    auto device = allocator.getAllocatorInfo().device;
    device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
        .objectType = vk::ObjectType::eImage,
        .objectHandle = reinterpret_cast<uint64_t>(&*image),
        .pObjectName = name.data()});
}

AllocatedBuffer::AllocatedBuffer(AllocatedBuffer&& other) {
    std::swap(buffer, other.buffer);
    std::swap(allocation, other.allocation);
    std::swap(allocator, other.allocator);
}

AllocatedBuffer::~AllocatedBuffer() {
    if (allocator)
        allocator.destroyBuffer(buffer, allocation);
}

AllocatedBuffer& AllocatedBuffer::operator=(AllocatedBuffer&& other) {
    std::swap(buffer, other.buffer);
    std::swap(allocation, other.allocation);
    std::swap(allocator, other.allocator);
    return *this;
}

AllocatedBuffer::AllocatedBuffer(
    vk::BufferCreateInfo create_info,
    vma::AllocationCreateInfo alloc_info,
    vma::Allocator allocator_,
    const std::string& name
) {
    allocator = allocator_;
    check_vk_result(allocator.createBuffer(
        &create_info,
        &alloc_info,
        &buffer,
        &allocation,
        nullptr
    ));
    auto device = allocator.getAllocatorInfo().device;
    device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
        .objectType = vk::ObjectType::eBuffer,
        .objectHandle = reinterpret_cast<uint64_t>(&*buffer),
        .pObjectName = name.data()});
}
