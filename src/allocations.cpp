#include "allocations.h"
#include "util.h"

AllocatedImage::AllocatedImage(
        vk::ImageCreateInfo create_info,
        vma::Allocator allocator_,
        const char* name
    ) {
        allocator = allocator_;
        vma::AllocationCreateInfo alloc_info = {
            .usage = vma::MemoryUsage::eAuto};
        check_vk_result(allocator.createImage(
            &create_info,
            &alloc_info,
            &image,
            &allocation,
            nullptr
        ));

        if (name) {
            auto device = allocator.getAllocatorInfo().device;
            device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
                .objectType = vk::ObjectType::eImage,
                .objectHandle = reinterpret_cast<uint64_t>(&*image),
                .pObjectName = name});
        }
    }

AllocatedBuffer::AllocatedBuffer(
        vk::BufferCreateInfo create_info,
        vma::AllocationCreateInfo alloc_info,
        vma::Allocator allocator_,
        const char* name
    ) {
        allocator = allocator_;
        check_vk_result(allocator.createBuffer(
            &create_info,
            &alloc_info,
            &buffer,
            &allocation,
            nullptr
        ));
        if (name) {
            auto device = allocator.getAllocatorInfo().device;
            device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT {
                .objectType = vk::ObjectType::eBuffer,
                .objectHandle = reinterpret_cast<uint64_t>(&*buffer),
                .pObjectName = name});
        }
    }
