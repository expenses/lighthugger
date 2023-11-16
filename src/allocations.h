
struct AllocatedImage {
    vk::Image image;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedImage(AllocatedImage&& other) {
        std::swap(image, other.image);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
    }

    AllocatedImage(vk::ImageCreateInfo create_info, vma::Allocator allocator_, const char* name = nullptr) {
        allocator = allocator_;
        vma::AllocationCreateInfo alloc_info = {.requiredFlags = vk::MemoryPropertyFlagBits::eDeviceLocal};
        auto err = allocator.createImage(&create_info, &alloc_info, &image, &allocation, nullptr);

        if (name) {
            auto device = allocator.getAllocatorInfo().device;
            device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{
                .objectType = vk::ObjectType::eImage,
                .objectHandle = reinterpret_cast<uint64_t>(&*image),
                .pObjectName = name
            });
        }
    }

    AllocatedImage& operator=(AllocatedImage&& other) {
        std::swap(image, other.image);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
        return *this;
    }

    ~AllocatedImage() {
        if (allocator) allocator.destroyImage(image, allocation);
    }
};

struct AllocatedBuffer {
    vk::Buffer buffer;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedBuffer(AllocatedBuffer&& other) {
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
    }

    AllocatedBuffer(vk::BufferCreateInfo create_info, vma::AllocationCreateInfo alloc_info, vma::Allocator allocator_, const char* name = nullptr) {
        allocator = allocator_;
        auto err = allocator.createBuffer(&create_info, &alloc_info, &buffer, &allocation, nullptr);
        if (name) {
            auto device = allocator.getAllocatorInfo().device;
            device.setDebugUtilsObjectNameEXT(vk::DebugUtilsObjectNameInfoEXT{
                .objectType = vk::ObjectType::eBuffer,
                .objectHandle = reinterpret_cast<uint64_t>(&*buffer),
                .pObjectName = name
            });
        }
    }

    AllocatedBuffer(vk::BufferCreateInfo create_info, vma::Allocator allocator_): AllocatedBuffer(create_info, {.usage = vma::MemoryUsage::eAuto}, allocator_) {}

    ~AllocatedBuffer() {
        if (allocator) allocator.destroyBuffer(buffer, allocation);
    }

    AllocatedBuffer& operator=(AllocatedBuffer&& other) {
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
        return *this;
    }
};
