
















#ifndef NATIVEWINDOW_GONKNATIVEWINDOW_LL_H
#define NATIVEWINDOW_GONKNATIVEWINDOW_LL_H

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include "GonkConsumerBaseLL.h"
#include "IGonkGraphicBufferConsumerLL.h"

namespace android {



class GonkNativeWindowNewFrameCallback {
public:
    virtual void OnNewFrame() = 0;
};







class GonkNativeWindow: public GonkConsumerBase
{
    typedef mozilla::layers::TextureClient TextureClient;
  public:
    typedef GonkConsumerBase::FrameAvailableListener FrameAvailableListener;
    typedef GonkBufferQueue::BufferItem BufferItem;

    enum { DEFAULT_MAX_BUFFERS = -1 };
    enum { INVALID_BUFFER_SLOT = GonkBufferQueue::INVALID_BUFFER_SLOT };
    enum { NO_BUFFER_AVAILABLE = GonkBufferQueue::NO_BUFFER_AVAILABLE };

    
    
    
    
    
    
    GonkNativeWindow(const sp<IGonkGraphicBufferConsumer>& consumer,
            int bufferCount = DEFAULT_MAX_BUFFERS);
    GonkNativeWindow(const sp<IGonkGraphicBufferConsumer>& consumer,
            uint32_t consumerUsage, int bufferCount = DEFAULT_MAX_BUFFERS,
            bool controlledByApp = false);

    virtual ~GonkNativeWindow();

    
    
    void setName(const String8& name);

    
    
    
    
    
    
    
    
    
    
    
    
    status_t acquireBuffer(BufferItem *item, nsecs_t presentWhen,
        bool waitForFence = true);

    
    
    
    
    
    
    status_t releaseBuffer(const BufferItem &item,
            const sp<Fence>& releaseFence = Fence::NO_FENCE);

    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    already_AddRefed<TextureClient> getCurrentBuffer();

    
    
    void returnBuffer(TextureClient* client);

    already_AddRefed<TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    void setNewFrameCallback(GonkNativeWindowNewFrameCallback* callback);

    static void RecycleCallback(TextureClient* client, void* closure);

protected:
#if ANDROID_VERSION == 21
    virtual void onFrameAvailable();
#else
    virtual void onFrameAvailable(const ::android::BufferItem &item);
#endif

private:
    GonkNativeWindowNewFrameCallback* mNewFrameCallback;
};

} 

#endif 
