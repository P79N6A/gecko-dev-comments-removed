
















#ifndef NATIVEWINDOW_GONKNATIVEWINDOW_JB_H
#define NATIVEWINDOW_GONKNATIVEWINDOW_JB_H

#include <ui/GraphicBuffer.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include "CameraCommon.h"
#include "GonkConsumerBaseJB.h"
#include "GrallocImages.h"
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

    typedef GonkBufferQueue::BufferItem BufferItem;

    enum { INVALID_BUFFER_SLOT = GonkBufferQueue::INVALID_BUFFER_SLOT };
    enum { NO_BUFFER_AVAILABLE = GonkBufferQueue::NO_BUFFER_AVAILABLE };

    
    
    
    
    GonkNativeWindow(int bufferCount = GonkBufferQueue::MIN_UNDEQUEUED_BUFFERS);

    virtual ~GonkNativeWindow();

    
    
    void setName(const String8& name);

    
    
    
    
    
    
    
    
    
    
    
    
#if ANDROID_VERSION >= 18
    status_t acquireBuffer(BufferItem *item, bool waitForFence = true);
#endif
    
    
    
    
    
    
#if ANDROID_VERSION >= 18
    status_t releaseBuffer(const BufferItem &item,
            const sp<Fence>& releaseFence = Fence::NO_FENCE);
#endif

    sp<IGraphicBufferProducer> getProducerInterface() const { return getBufferQueue(); }

    
    
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    
    
    
    status_t setDefaultBufferFormat(uint32_t defaultFormat);

    
    already_AddRefed<TextureClient> getCurrentBuffer();

    
    
    void returnBuffer(TextureClient* client);

    already_AddRefed<TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    void setNewFrameCallback(GonkNativeWindowNewFrameCallback* callback);

    static void RecycleCallback(TextureClient* client, void* closure);

protected:
    virtual void onFrameAvailable();

private:
    GonkNativeWindowNewFrameCallback* mNewFrameCallback;
};

} 

#endif 
