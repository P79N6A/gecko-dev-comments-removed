









#include "webrtc/modules/desktop_capture/shared_memory.h"

namespace webrtc {

#if defined(WEBRTC_WIN)
const SharedMemory::Handle SharedMemory::kInvalidHandle = NULL;
#else
const SharedMemory::Handle SharedMemory::kInvalidHandle = -1;
#endif

SharedMemory::SharedMemory(void* data, size_t size, Handle handle, int id)
  : data_(data),
    size_(size),
    handle_(handle),
    id_(id) {
}

}  
