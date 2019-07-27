
















#include "GonkBufferSlot.h"

namespace android {

const char* GonkBufferSlot::bufferStateName(BufferState state) {
    switch (state) {
        case GonkBufferSlot::DEQUEUED: return "DEQUEUED";
        case GonkBufferSlot::QUEUED: return "QUEUED";
        case GonkBufferSlot::FREE: return "FREE";
        case GonkBufferSlot::ACQUIRED: return "ACQUIRED";
        default: return "Unknown";
    }
}

} 
