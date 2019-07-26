
















#ifndef DOM_CAMERA_GONKNATIVEWINDOW_H
#define DOM_CAMERA_GONKNATIVEWINDOW_H

#include <stdint.h>
#include <sys/types.h>

#include <gui/ISurfaceTexture.h>
#include <ui/egl/android_natives.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/threads.h>

#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "GonkIOSurfaceImage.h"
#include "CameraCommon.h"

namespace android {



class GonkNativeWindowNewFrameCallback {
public:
    virtual void OnNewFrame() = 0;
};

class GonkNativeWindow : public BnSurfaceTexture
{
    friend class GonkNativeWindowClient;

    typedef mozilla::layers::SurfaceDescriptor SurfaceDescriptor;
    typedef mozilla::layers::GraphicBufferLocked GraphicBufferLocked;

public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { MIN_BUFFER_SLOTS = MIN_UNDEQUEUED_BUFFERS };
    enum { NUM_BUFFER_SLOTS = 32 };
    enum { NO_CONNECTED_API = 0 };
    enum { NATIVE_WINDOW_SET_BUFFERS_SIZE = 0x10000000 };

    GonkNativeWindow();
    ~GonkNativeWindow(); 

    
    
    already_AddRefed<GraphicBufferLocked> getCurrentBuffer();

    
    
    bool returnBuffer(uint32_t index, uint32_t generation);

    
    
    
    virtual status_t setBufferCount(int bufferCount);

    
    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);

    
    
    
    
    
    virtual status_t dequeueBuffer(int *buf, uint32_t width, uint32_t height,
            uint32_t format, uint32_t usage);

    
    
    
    
    
    virtual status_t queueBuffer(int buf, int64_t timestamp,
            uint32_t* outWidth, uint32_t* outHeight, uint32_t* outTransform);
    virtual void cancelBuffer(int buf);
    virtual status_t setCrop(const Rect& reg);
    virtual status_t setTransform(uint32_t transform);
    virtual status_t setScalingMode(int mode);

    virtual int query(int what, int* value);

    
    virtual int performQcomOperation(int operation, int arg1, int arg2, int arg3) {
        return OK;
    }

    
    
    virtual status_t setSynchronousMode(bool enabled);

    
    
    
    
    virtual status_t connect(int api,
            uint32_t* outWidth, uint32_t* outHeight, uint32_t* outTransform);

    
    
    
    virtual status_t disconnect(int api);

    void setNewFrameCallback(GonkNativeWindowNewFrameCallback* aCallback);

    
    
    
    
    status_t setDefaultBufferSize(uint32_t width, uint32_t height);

    
    
    
    
    
    
    
    
    
    void abandon();

    SurfaceDescriptor *getSurfaceDescriptorFromBuffer(ANativeWindowBuffer* buffer);

protected:

    
    
    
    void freeAllBuffersLocked(nsTArray<SurfaceDescriptor>& freeList);

    
    
    void releaseBufferFreeListUnlocked(nsTArray<SurfaceDescriptor>& freeList);

    
    
    
    
    void clearRenderingStateBuffersLocked();

private:
    void init();

    int getSlotFromBufferLocked(android_native_buffer_t* buffer) const;

    enum { INVALID_BUFFER_SLOT = -1 };

    struct BufferSlot {

        BufferSlot()
            : mBufferState(BufferSlot::FREE),
              mTimestamp(0),
              mFrameNumber(0){
        }

        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        SurfaceDescriptor mSurfaceDescriptor;

        
        
        enum BufferState {
            
            
            
            FREE = 0,

            
            
            
            
            
            
            
            
            
            
            DEQUEUED = 1,

            
            
            
            
            
            
            
            QUEUED = 2,

            
            
            
            
            RENDERING = 3,
        };

        
        BufferState mBufferState;

        
        
        
        bool mRequestBufferCalled;

        
        
        int64_t mTimestamp;

        
        uint64_t mFrameNumber;
    };

    
    
    
    
    
    BufferSlot mSlots[NUM_BUFFER_SLOTS];

    
    mutable Condition mDequeueCondition;

    
    
    
    
    
    bool mAbandoned;

    
    
    
    int64_t mTimestamp;

    
    
    uint32_t mDefaultWidth;

    
    
    uint32_t mDefaultHeight;

    
    
    uint32_t mPixelFormat;

    
    
    
    int mBufferCount;

    
    
    
    int mConnectedApi;

    
    
    typedef Vector<int> Fifo;
    Fifo mQueue;

    
    
    
    mutable Mutex mMutex;

    
    uint64_t mFrameCounter;

    
    uint32_t mGeneration;

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

}; 

#endif
