





#ifndef jscrashreport_h
#define jscrashreport_h

#include "mozilla/GuardObjects.h"

#include <stdint.h>

namespace js {
namespace crash {

void
SnapshotGCStack();

void
SnapshotErrorStack();

void
SaveCrashData(uint64_t tag, void *ptr, size_t size);

template<size_t size, unsigned char marker>
class StackBuffer
{
  public:
    explicit StackBuffer(void *data
                         MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;

        buffer[0] = marker;
        buffer[1] = '[';

        for (size_t i = 0; i < size; i++) {
            if (data)
                buffer[i + 2] = static_cast<unsigned char*>(data)[i];
            else
                buffer[i + 2] = 0;
        }

        buffer[size - 2] = ']';
        buffer[size - 1] = marker;
    }

  private:
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
    volatile unsigned char buffer[size + 4];
};

} 
} 

#endif 
