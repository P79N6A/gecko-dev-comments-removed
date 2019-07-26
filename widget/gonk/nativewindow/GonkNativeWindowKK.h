
















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
    typedef mozilla::layers::GraphicBufferLocked GraphicBufferLocked;
    typedef mozilla::layers::SurfaceDescriptor SurfaceDescriptor;
  public:
    typedef GonkConsumerBase::FrameAvailableListener FrameAvailableListener;
    typedef IGonkGraphicBufferConsumer::BufferItem BufferItem;

    enum { INVALID_BUFFER_SLOT = GonkBufferQueue::INVALID_BUFFER_SLOT };
    enum { NO_BUFFER_AVAILABLE = GonkBufferQueue::NO_BUFFER_AVAILABLE };

    
    
    
    
    
    
    GonkNativeWindow();
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

    
    already_AddRefed<GraphicBufferLocked> getCurrentBuffer();

    
    
    bool returnBuffer(uint32_t index, uint32_t generation);

    SurfaceDescriptor* getSurfaceDescriptorFromBuffer(ANativeWindowBuffer* buffer);

    void setNewFrameCallback(GonkNativeWindowNewFrameCallback* aCallback);

protected:
    virtual void onFrameAvailable();

private:
    GonkNativeWindowNewFrameCallback* mNewFrameCallback;
};


class CameraGraphicBuffer : public mozilla::layers::GraphicBufferLocked
{
    typedef mozilla::layers::SurfaceDescriptor SurfaceDescriptor;
    typedef mozilla::layers::ImageBridgeChild ImageBridgeChild;

public:
    CameraGraphicBuffer(GonkNativeWindow* aNativeWindow,
                        uint32_t aIndex,
                        uint32_t aGeneration,
                        SurfaceDescriptor aBuffer)
        : GraphicBufferLocked(aBuffer)
        , mNativeWindow(aNativeWindow)
        , mIndex(aIndex)
        , mGeneration(aGeneration)
        , mLocked(true)
    {
        DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
    }

    virtual ~CameraGraphicBuffer()
    {
        DOM_CAMERA_LOGT("%s:%d : this=%p\n", __func__, __LINE__, this);
    }

    
    
    virtual void Unlock() MOZ_OVERRIDE
    {
        if (mLocked) {
            
            
            sp<GonkNativeWindow> window = mNativeWindow.promote();
            if (window.get() && window->returnBuffer(mIndex, mGeneration)) {
                mLocked = false;
            } else {
                
                
                ImageBridgeChild *ibc = ImageBridgeChild::GetSingleton();
                ibc->DeallocSurfaceDescriptorGralloc(mSurfaceDescriptor);
            }
        }
    }

protected:
    wp<GonkNativeWindow> mNativeWindow;
    uint32_t mIndex;
    uint32_t mGeneration;
    bool mLocked;
};

} 

#endif 
