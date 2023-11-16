
struct AllocatedImage {
    vk::Image image;
    vma::Allocation allocation;
    vma::Allocator allocator;

    AllocatedImage(vk::ImageCreateInfo create_info, vma::Allocator allocator_) {
        allocator = allocator_;
        vma::AllocationCreateInfo alloc_info = {.usage = vma::MemoryUsage::eAuto};
        auto err = allocator.createImage(&create_info, &alloc_info, &image, &allocation, nullptr);
    }

    ~AllocatedImage() {
        allocator.destroyImage(image, allocation);
    }

    // https://radekvit.medium.com/move-semantics-in-c-and-rust-the-case-for-destructive-moves-d816891c354b
    AllocatedImage& operator=(AllocatedImage&& other) {
        std::swap(image, other.image);
        std::swap(allocation, other.allocation);
        std::swap(allocator, other.allocator);
        return *this;
    }
};
