#include "persistently_mapped.h"

PersistentlyMappedBuffer::PersistentlyMappedBuffer(AllocatedBuffer buffer_) :
        buffer(std::move(buffer_)) {
        auto buffer_info =
            buffer.allocator.getAllocationInfo(buffer.allocation);
        mapped_ptr = buffer_info.pMappedData;
        assert(mapped_ptr);
    }
