















#ifndef ANDROID_GUI_BUFFERITEMCONSUMER_H
#define ANDROID_GUI_BUFFERITEMCONSUMER_H

#include <gui/ConsumerBase.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#define ANDROID_GRAPHICS_BUFFERITEMCONSUMER_JNI_ID "mBufferItemConsumer"

namespace android {

class BufferQueue;







class BufferItemConsumer: public ConsumerBase
{
  public:
    typedef ConsumerBase::FrameAvailableListener FrameAvailableListener;

    typedef BufferQueue::BufferItem BufferItem;

    enum { DEFAULT_MAX_BUFFERS = -1 };
    enum { INVALID_BUFFER_SLOT = BufferQueue::INVALID_BUFFER_SLOT };
    enum { NO_BUFFER_AVAILABLE = BufferQueue::NO_BUFFER_AVAILABLE };

    
    
    
    
    
    
    BufferItemConsumer(const sp<IGraphicBufferConsumer>& consumer,
            uint32_t consumerUsage, int bufferCount = DEFAULT_MAX_BUFFERS,
            bool controlledByApp = false);

    virtual ~BufferItemConsumer();

    
    
    void setName(const String8& name);

    
    
    
    
    
    
    
    
    
    
    
    
    status_t acquireBuffer(BufferItem *item, nsecs_t presentWhen,
        bool waitForFence = true);

    
    
    
    
    
    
    status_t releaseBuffer(const BufferItem &item,
            const sp<Fence>& releaseFence = Fence::NO_FENCE);

    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);
};

} 

#endif 
