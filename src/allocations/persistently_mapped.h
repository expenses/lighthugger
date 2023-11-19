#include "base.h"

struct PersistentlyMappedBuffer {
    AllocatedBuffer buffer;
    void* mapped_ptr;

    PersistentlyMappedBuffer(AllocatedBuffer buffer_) :
        buffer(std::move(buffer_)) {
        auto buffer_info =
            buffer.allocator.getAllocationInfo(buffer.allocation);
        mapped_ptr = buffer_info.pMappedData;
        assert(mapped_ptr);
    }
};
