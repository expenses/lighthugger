#pragma once
#include "base.h"

struct PersistentlyMappedBuffer {
    AllocatedBuffer buffer;
    void* mapped_ptr;

    PersistentlyMappedBuffer(AllocatedBuffer buffer_);
};
