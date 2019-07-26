















#ifndef ANDROID_FENCE_H
#define ANDROID_FENCE_H

#include <stdint.h>
#include <sys/types.h>

#include <ui/ANativeObjectBase.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <utils/Flattenable.h>
#include <utils/String8.h>
#include <utils/Timers.h>

struct ANativeWindowBuffer;

namespace android {





class Fence
    : public LightRefBase<Fence>, public Flattenable
{
public:
    static const sp<Fence> NO_FENCE;

    
    
    enum { TIMEOUT_NEVER = -1 };

    
    
    
    Fence();

    
    
    
    Fence(int fenceFd);

    
    
    
    bool isValid() const { return mFenceFd != -1; }

    
    
    
    
    
    status_t wait(unsigned int timeout);

    
    
    
    
    
    
    status_t waitForever(const char* logname);

    
    
    
    
    static sp<Fence> merge(const String8& name, const sp<Fence>& f1,
            const sp<Fence>& f2);

    
    
    
    int dup() const;

    
    
    
    
    nsecs_t getSignalTime() const;

    
    size_t getFlattenedSize() const;
    size_t getFdCount() const;
    status_t flatten(void* buffer, size_t size,
            int fds[], size_t count) const;
    status_t unflatten(void const* buffer, size_t size,
            int fds[], size_t count);

private:
    
    friend class LightRefBase<Fence>;
    virtual ~Fence();

    
    Fence(const Fence& rhs);
    Fence& operator = (const Fence& rhs);
    const Fence& operator = (const Fence& rhs) const;

    int mFenceFd;
};

}; 

#endif 
