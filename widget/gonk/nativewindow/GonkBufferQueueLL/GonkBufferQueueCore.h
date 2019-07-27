
















#ifndef NATIVEWINDOW_GONKBUFFERQUEUECORE_LL_H
#define NATIVEWINDOW_GONKBUFFERQUEUECORE_LL_H

#include "GonkBufferQueueDefs.h"
#include "GonkBufferSlot.h"

#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/NativeHandle.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>
#include <utils/Trace.h>
#include <utils/Vector.h>

#include "mozilla/layers/TextureClient.h"

#define ATRACE_BUFFER_INDEX(index)

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;

namespace android {

class GonkBufferItem;
class IConsumerListener;
class IGraphicBufferAlloc;
class IProducerListener;

class GonkBufferQueueCore : public virtual RefBase {

    friend class GonkBufferQueueProducer;
    friend class GonkBufferQueueConsumer;

public:
    
    
    enum { INVALID_BUFFER_SLOT = -1 }; 

    
    
    enum { MAX_MAX_ACQUIRED_BUFFERS = GonkBufferQueueDefs::NUM_BUFFER_SLOTS - 2 };

    
    enum { NO_CONNECTED_API = 0 };

    typedef Vector<GonkBufferItem> Fifo;
    typedef mozilla::layers::TextureClient TextureClient;

    
    
    
    GonkBufferQueueCore(const sp<IGraphicBufferAlloc>& allocator = NULL);
    virtual ~GonkBufferQueueCore();

private:
    
    void dump(String8& result, const char* prefix) const;

    int getSlotFromTextureClientLocked(TextureClient* client) const;

    
    
    
    int getMinUndequeuedBufferCountLocked(bool async) const;

    
    
    
    int getMinMaxBufferCountLocked(bool async) const;

    
    
    
    
    
    
    
    
    
    
    
    int getMaxBufferCountLocked(bool async) const;

    
    
    
    
    status_t setDefaultMaxBufferCountLocked(int count);

    
    
    void freeBufferLocked(int slot);

    
    
    void freeAllBuffersLocked();

    
    
    bool stillTracking(const GonkBufferItem* item) const;

    
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

    
    
    
    
    
    GonkBufferQueueDefs::SlotsType mSlots;

    
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
