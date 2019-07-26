
















#ifndef NATIVEWINDOW_GONKNATIVEWINDOW_KK_H
#define NATIVEWINDOW_GONKNATIVEWINDOW_KK_H

#include <ui/GraphicBuffer.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include "CameraCommon.h"
#include "GonkConsumerBaseKK.h"
#include "GrallocImages.h"
#include "IGonkGraphicBufferConsumer.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "mozilla/layers/LayersSurfaces.h"

namespace mozilla {
namespace layers {
    class PGrallocBufferChild;
}
}

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
    typedef IGonkGraphicBufferConsumer::BufferItem BufferItem;

    enum { INVALID_BUFFER_SLOT = GonkBufferQueue::INVALID_BUFFER_SLOT };
    enum { NO_BUFFER_AVAILABLE = GonkBufferQueue::NO_BUFFER_AVAILABLE };

    
    
    
    
    
    
    GonkNativeWindow(int bufferCount = GonkBufferQueue::MIN_UNDEQUEUED_BUFFERS);
    GonkNativeWindow(const sp<GonkBufferQueue>& bq, uint32_t consumerUsage,
            int bufferCount = GonkBufferQueue::MIN_UNDEQUEUED_BUFFERS,
            bool controlledByApp = false);

    virtual ~GonkNativeWindow();

    
    
    void setName(const String8& name);

    
    
    
    
    
    
    
    
    
    
    
    
    status_t acquireBuffer(BufferItem *item, nsecs_t presentWhen,
        bool waitForFence = true);

    
    
    
    
    
    
    status_t releaseBuffer(const BufferItem &item,
            const sp<Fence>& releaseFence = Fence::NO_FENCE);

    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    mozilla::TemporaryRef<TextureClient> getCurrentBuffer();

    
    
    void returnBuffer(TextureClient* client);

    mozilla::TemporaryRef<TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    void setNewFrameCallback(GonkNativeWindowNewFrameCallback* callback);

    static void RecycleCallback(TextureClient* client, void* closure);

protected:
    virtual void onFrameAvailable();

private:
    GonkNativeWindowNewFrameCallback* mNewFrameCallback;
};

} 

#endif 
