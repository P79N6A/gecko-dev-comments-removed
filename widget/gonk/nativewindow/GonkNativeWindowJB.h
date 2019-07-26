
















#ifndef NATIVEWINDOW_GONKNATIVEWINDOW_JB_H
#define NATIVEWINDOW_GONKNATIVEWINDOW_JB_H

#include <gui/ConsumerBase.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#define ANDROID_GRAPHICS_BUFFERITEMCONSUMER_JNI_ID "mBufferItemConsumer"

namespace android {







class GonkNativeWindow: public ConsumerBase
{
  public:
    typedef ConsumerBase::FrameAvailableListener FrameAvailableListener;

    typedef BufferQueue::BufferItem BufferItem;

    enum { INVALID_BUFFER_SLOT = BufferQueue::INVALID_BUFFER_SLOT };
    enum { NO_BUFFER_AVAILABLE = BufferQueue::NO_BUFFER_AVAILABLE };

    
    
    
    
    GonkNativeWindow(uint32_t consumerUsage,
            int bufferCount = BufferQueue::MIN_UNDEQUEUED_BUFFERS,
            bool synchronousMode = false);

    virtual ~GonkNativeWindow();

    
    
    void setName(const String8& name);

    
    
    
    
    
    
    
    
    
    
    
    
    status_t acquireBuffer(BufferItem *item, bool waitForFence = true);

    
    
    
    
    
    
    status_t releaseBuffer(const BufferItem &item,
            const sp<Fence>& releaseFence = Fence::NO_FENCE);

    sp<IGraphicBufferProducer> getProducerInterface() const { return getBufferQueue(); }

    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);
};

} 

#endif 
