
struct AllocatedImage {
    vk::Image image;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedImage(AllocatedImage&& other) {
        std::swap(image, other.image);
        std::swap(allocation, other.allocation);
        // Swapping the allocator will cause the moved-from `other`'s
        // allocator to be set to nullptr, which causes a segfault
        // when trying to destroy an image. Just copying the value means that
        // destroyImage is called with a valid allocator but image and allocation set to
        // nullptr, which does nothing.
        allocator = other.allocator;
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
        allocator.destroyImage(image, allocation);
    }
};

struct AllocatedBuffer {
    vk::Buffer buffer;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedBuffer(AllocatedBuffer&& other) {
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        allocator = other.allocator;
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
        allocator.destroyBuffer(buffer, allocation);
    }

    AllocatedBuffer& operator=(AllocatedBuffer&& other) {
        std::swap(buffer, other.buffer);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
        return *this;
    }
};
