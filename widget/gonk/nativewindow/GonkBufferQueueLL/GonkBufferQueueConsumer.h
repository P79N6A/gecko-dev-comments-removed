
















#ifndef NATIVEWINDOW_GONKBUFFERQUEUECONSUMER_LL_H
#define NATIVEWINDOW_GONKBUFFERQUEUECONSUMER_LL_H

#include "GonkBufferQueueDefs.h"
#include "IGonkGraphicBufferConsumerLL.h"

namespace android {

class GonkBufferQueueCore;

class GonkBufferQueueConsumer : public BnGonkGraphicBufferConsumer {

public:
    GonkBufferQueueConsumer(const sp<GonkBufferQueueCore>& core);
    virtual ~GonkBufferQueueConsumer();

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t acquireBuffer(BufferItem* outBuffer,
            nsecs_t expectedPresent);

    
    virtual status_t detachBuffer(int slot);

    
    virtual status_t attachBuffer(int* slot, const sp<GraphicBuffer>& buffer);

    
    
    
    
    
    
    
    
    
    virtual status_t releaseBuffer(int slot, uint64_t frameNumber,
            const sp<Fence>& releaseFence);

    
    
    
    
    
    
    
    
    virtual status_t connect(const sp<IConsumerListener>& consumerListener,
            bool controlledByApp);

    
    
    
    
    virtual status_t disconnect();

    
    
    
    
    
    virtual status_t getReleasedBuffers(uint64_t* outSlotMask);

    
    
    
    virtual status_t setDefaultBufferSize(uint32_t width, uint32_t height);

    
    
    
    
    
    
    virtual status_t setDefaultMaxBufferCount(int bufferCount);

    
    
    
    
    
    virtual status_t disableAsyncBuffer();

    
    
    
    virtual status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers);

    
    virtual void setConsumerName(const String8& name);

    
    
    
    
    virtual status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    
    
    virtual status_t setConsumerUsageBits(uint32_t usage);

    
    
    
    virtual status_t setTransformHint(uint32_t hint);

    
    virtual sp<NativeHandle> getSidebandStream() const;

    
    virtual void dumpToString(String8& result, const char* prefix) const;

    
    virtual already_AddRefed<GonkBufferSlot::TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    virtual int getSlotFromTextureClientLocked(GonkBufferSlot::TextureClient* client) const;

    
    
    
    virtual status_t consumerConnect(const sp<IConsumerListener>& consumer,
            bool controlledByApp) {
        return connect(consumer, controlledByApp);
    }

    virtual status_t consumerDisconnect() { return disconnect(); }

    

private:
    sp<GonkBufferQueueCore> mCore;

    
    GonkBufferQueueDefs::SlotsType& mSlots;

    
    
    String8 mConsumerName;

}; 

} 

#endif
