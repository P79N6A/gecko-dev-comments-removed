















#ifndef ANDROID_GUI_BUFFERQUEUECORE_H
#define ANDROID_GUI_BUFFERQUEUECORE_H

#include <gui/BufferQueueDefs.h>
#include <gui/BufferSlot.h>

#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/NativeHandle.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>
#include <utils/Trace.h>
#include <utils/Vector.h>

#define BQ_LOGV(x, ...) ALOGV("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define BQ_LOGD(x, ...) ALOGD("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define BQ_LOGI(x, ...) ALOGI("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define BQ_LOGW(x, ...) ALOGW("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)
#define BQ_LOGE(x, ...) ALOGE("[%s] "x, mConsumerName.string(), ##__VA_ARGS__)

#define ATRACE_BUFFER_INDEX(index)                                   \
    if (ATRACE_ENABLED()) {                                          \
        char ___traceBuf[1024];                                      \
        snprintf(___traceBuf, 1024, "%s: %d",                        \
                mCore->mConsumerName.string(), (index));             \
        android::ScopedTrace ___bufTracer(ATRACE_TAG, ___traceBuf);  \
    }

namespace android {

class BufferItem;
class IConsumerListener;
class IGraphicBufferAlloc;
class IProducerListener;

class BufferQueueCore : public virtual RefBase {

    friend class BufferQueueProducer;
    friend class BufferQueueConsumer;

public:
    
    
    enum { INVALID_BUFFER_SLOT = -1 }; 

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = BufferQueueDefs::NUM_BUFFER_SLOTS - 2 };

    
    enum { NO_CONNECTED_API = 0 };

    typedef Vector<BufferItem> Fifo;

    
    
    
    BufferQueueCore(const sp<IGraphicBufferAlloc>& allocator = NULL);
    virtual ~BufferQueueCore();

private:
    
    void dump(String8& result, const char* prefix) const;

    
    
    
    int getMinUndequeuedBufferCountLocked(bool async) const;

    
    
    
    int getMinMaxBufferCountLocked(bool async) const;

    
    
    
    
    
    
    
    
    
    
    
    int getMaxBufferCountLocked(bool async) const;

    
    
    
    
    status_t setDefaultMaxBufferCountLocked(int count);

    
    
    void freeBufferLocked(int slot);

    
    
    void freeAllBuffersLocked();

    
    
    bool stillTracking(const BufferItem* item) const;

    
    void waitWhileAllocatingLocked() const;

    
    
    sp<IGraphicBufferAlloc> mAllocator;

    
    
    
    mutable Mutex mMutex;

    
    
    
    
    
    
    bool mIsAbandoned;

    
    
    bool mConsumerControlledByApp;

    
    
    
    String8 mConsumerName;

    
    
    
    sp<IConsumerListener> mConsumerListener;

    
    
    uint32_t mConsumerUsageBits;

    
    
    
    int mConnectedApi;

    
    
    sp<IProducerListener> mConnectedProducerListener;

    
    
    
    
    
    BufferQueueDefs::SlotsType mSlots;

    
    Fifo mQueue;

    
    
    
    
    
    int mOverrideMaxBufferCount;

    
    
    mutable Condition mDequeueCondition;

    
    
    bool mUseAsyncBuffer;

    
    
    
    bool mDequeueBufferCannotBlock;

    
    
    uint32_t mDefaultBufferFormat;

    
    
    int mDefaultWidth;

    
    
    int mDefaultHeight;

    
    
    
    
    int mDefaultMaxBufferCount;

    
    
    
    
    
    int mMaxAcquiredBufferCount;

    
    
    
    bool mBufferHasBeenQueued;

    
    
    uint64_t mFrameCounter;

    
    uint32_t mTransformHint;

    
    sp<NativeHandle> mSidebandStream;

    
    
    
    
    bool mIsAllocating;

    
    
    mutable Condition mIsAllocatingCondition;
}; 

} 

#endif
