
















#ifndef NATIVEWINDOW_GONKBUFFERQUEUEPRODUCER_LL_H
#define NATIVEWINDOW_GONKBUFFERQUEUEPRODUCER_LL_H

#include "GonkBufferQueueDefs.h"
#include <gui/IGraphicBufferProducer.h>

namespace android {

class GonkBufferQueueProducer : public BnGraphicBufferProducer,
                            private IBinder::DeathRecipient {
public:
    friend class GonkBufferQueue; 

    GonkBufferQueueProducer(const sp<GonkBufferQueueCore>& core);
    virtual ~GonkBufferQueueProducer();

    
    
    
    
    
    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t setBufferCount(int bufferCount);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t dequeueBuffer(int *outSlot, sp<Fence>* outFence, bool async,
            uint32_t width, uint32_t height, uint32_t format, uint32_t usage);

    
    virtual status_t detachBuffer(int slot);

    
    virtual status_t detachNextBuffer(sp<GraphicBuffer>* outBuffer,
            sp<Fence>* outFence);

    
    virtual status_t attachBuffer(int* outSlot, const sp<GraphicBuffer>& buffer);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t queueBuffer(int slot,
            const QueueBufferInput& input, QueueBufferOutput* output);

    
    
    
    
    
    virtual void cancelBuffer(int slot, const sp<Fence>& fence);

    
    
    virtual int query(int what, int* outValue);

    
    
    
    
    
    
    
    
    
    virtual status_t connect(const sp<IProducerListener>& listener,
            int api, bool producerControlledByApp, QueueBufferOutput* output);

    
    
    
    
    
    
    
    
    virtual status_t disconnect(int api);

    
    
    
    
    
    
    
    
    
    
    virtual status_t setSidebandStream(const sp<NativeHandle>& stream);

    
    virtual void allocateBuffers(bool async, uint32_t width, uint32_t height,
            uint32_t format, uint32_t usage);

    
    
    
    
    
    
    
    
    virtual status_t setSynchronousMode(bool enabled);

private:
    
    virtual void binderDied(const wp<IBinder>& who);

    
    
    
    
    
    status_t waitForFreeSlotThenRelock(const char* caller, bool async,
            int* found, status_t* returnFlags) const;

    sp<GonkBufferQueueCore> mCore;

    
    GonkBufferQueueDefs::SlotsType& mSlots;

    
    
    
    String8 mConsumerName;

    
    bool mSynchronousMode;

    uint32_t mStickyTransform;

}; 

} 

#endif
