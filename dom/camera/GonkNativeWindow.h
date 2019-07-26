


















#ifndef DOM_CAMERA_GONKNATIVEWINDOW_H
#define DOM_CAMERA_GONKNATIVEWINDOW_H

#include <stdint.h>
#include <sys/types.h>

#include <ui/egl/android_natives.h>

#include <utils/Errors.h>
#include <utils/RefBase.h>

#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
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

class GonkNativeWindow : public EGLNativeBase<ANativeWindow, GonkNativeWindow, RefBase>
{
    typedef mozilla::layers::SurfaceDescriptor SurfaceDescriptor;
    typedef mozilla::layers::GraphicBufferLocked GraphicBufferLocked;

public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { MIN_BUFFER_SLOTS = MIN_UNDEQUEUED_BUFFERS };
    enum { NUM_BUFFER_SLOTS = 32 };
    enum { NATIVE_WINDOW_SET_BUFFERS_SIZE = 0x10000000 };

    GonkNativeWindow();
    GonkNativeWindow(GonkNativeWindowNewFrameCallback* aCallback);
    ~GonkNativeWindow(); 

    
    static int hook_cancelBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer);
    static int hook_lockBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_perform(ANativeWindow* window, int operation, ...);
    static int hook_query(const ANativeWindow* window, int what, int* value);
    static int hook_queueBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int hook_setSwapInterval(ANativeWindow* window, int interval);

    
    
    already_AddRefed<GraphicBufferLocked> getCurrentBuffer();

    
    
    bool returnBuffer(uint32_t index, uint32_t generation);

    
    void abandon();

    SurfaceDescriptor *getSurfaceDescriptorFromBuffer(ANativeWindowBuffer* buffer);

protected:
    virtual int cancelBuffer(ANativeWindowBuffer* buffer);
    virtual int dequeueBuffer(ANativeWindowBuffer** buffer);
    virtual int lockBuffer(ANativeWindowBuffer* buffer);
    virtual int perform(int operation, va_list args);
    virtual int query(int what, int* value) const;
    virtual int queueBuffer(ANativeWindowBuffer* buffer);
    virtual int setSwapInterval(int interval);

    virtual int setBufferCount(int bufferCount);
    virtual int setBuffersDimensions(int w, int h);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersTimestamp(int64_t timestamp);
    virtual int setUsage(uint32_t reqUsage);

    
    
    
    void freeAllBuffersLocked(nsTArray<SurfaceDescriptor>& freeList);

    
    
    void releaseBufferFreeListUnlocked(nsTArray<SurfaceDescriptor>& freeList);

private:
    void init();

    int dispatchSetBufferCount(va_list args);
    int dispatchSetBuffersGeometry(va_list args);
    int dispatchSetBuffersDimensions(va_list args);
    int dispatchSetBuffersFormat(va_list args);
    int dispatchSetBuffersTimestamp(va_list args);
    int dispatchSetUsage(va_list args);

    int getSlotFromBufferLocked(android_native_buffer_t* buffer) const;

private:
    enum { INVALID_BUFFER_SLOT = -1 };

    struct BufferSlot {

        BufferSlot()
            : mGraphicBuffer(0),
              mBufferState(BufferSlot::FREE),
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

        
        
        int64_t mTimestamp;

        
        uint64_t mFrameNumber;
    };

    
    
    
    
    
    BufferSlot mSlots[NUM_BUFFER_SLOTS];

    
    mutable Condition mDequeueCondition;

    
    
    
    int64_t mTimestamp;

    
    
    uint32_t mDefaultWidth;

    
    
    uint32_t mDefaultHeight;

    
    
    uint32_t mPixelFormat;

    
    uint32_t mUsage;

    
    
    
    int mBufferCount;

    
    
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
