















#ifndef ANDROID_GUI_BUFFERQUEUECONSUMER_H
#define ANDROID_GUI_BUFFERQUEUECONSUMER_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gui/BufferQueueDefs.h>
#include <gui/IGraphicBufferConsumer.h>

namespace android {

class BufferQueueCore;

class BufferQueueConsumer : public BnGraphicBufferConsumer {

public:
    BufferQueueConsumer(const sp<BufferQueueCore>& core);
    virtual ~BufferQueueConsumer();

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t acquireBuffer(BufferItem* outBuffer,
            nsecs_t expectedPresent);

    
    virtual status_t detachBuffer(int slot);

    
    virtual status_t attachBuffer(int* slot, const sp<GraphicBuffer>& buffer);

    
    
    
    
    
    
    
    
    
    
    
    
    virtual status_t releaseBuffer(int slot, uint64_t frameNumber,
            const sp<Fence>& releaseFence, EGLDisplay display,
            EGLSyncKHR fence);

    
    
    
    
    
    
    
    
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

    
    virtual void dump(String8& result, const char* prefix) const;

    
    
    

    virtual status_t releaseBuffer(int buf, uint64_t frameNumber,
            EGLDisplay display, EGLSyncKHR fence,
            const sp<Fence>& releaseFence) {
        return releaseBuffer(buf, frameNumber, releaseFence, display, fence);
    }

    virtual status_t consumerConnect(const sp<IConsumerListener>& consumer,
            bool controlledByApp) {
        return connect(consumer, controlledByApp);
    }

    virtual status_t consumerDisconnect() { return disconnect(); }

    

private:
    sp<BufferQueueCore> mCore;

    
    BufferQueueDefs::SlotsType& mSlots;

    
    
    String8 mConsumerName;

}; 

} 

#endif
