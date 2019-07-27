
















#ifndef NATIVEWINDOW_GONKNATIVEWINDOW_ICS_H
#define NATIVEWINDOW_GONKNATIVEWINDOW_ICS_H

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

#include "CameraCommon.h"
#include "GrallocImages.h"
#include "mozilla/layers/LayersSurfaces.h"
#include "mozilla/layers/TextureClient.h"

namespace android {



class GonkNativeWindowNewFrameCallback {
public:
    virtual void OnNewFrame() = 0;
};

class GonkNativeWindow : public BnSurfaceTexture
{
    friend class GonkNativeWindowClient;

    typedef mozilla::layers::TextureClient TextureClient;

public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { MIN_BUFFER_SLOTS = MIN_UNDEQUEUED_BUFFERS };
    enum { NUM_BUFFER_SLOTS = 32 };
    enum { NO_CONNECTED_API = 0 };
    enum { NATIVE_WINDOW_SET_BUFFERS_SIZE = 0x10000000 };

    GonkNativeWindow();
    ~GonkNativeWindow(); 

    
    
    already_AddRefed<TextureClient> getCurrentBuffer();

    
    
    void returnBuffer(TextureClient* client);

    
    
    
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

    already_AddRefed<TextureClient> getTextureClientFromBuffer(ANativeWindowBuffer* buffer);

    static void RecycleCallback(TextureClient* client, void* closure);

protected:

    
    
    
    void freeAllBuffersLocked();

    
    
    
    
    void clearRenderingStateBuffersLocked();

private:
    void init();

    int getSlotFromBufferLocked(android_native_buffer_t* buffer) const;

    int getSlotFromTextureClientLocked(TextureClient* client) const;

    enum { INVALID_BUFFER_SLOT = -1 };

    struct BufferSlot {

        BufferSlot()
            : mBufferState(BufferSlot::FREE),
              mTimestamp(0),
              mFrameNumber(0){
        }

        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        mozilla::RefPtr<TextureClient> mTextureClient;

        
        
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

    GonkNativeWindowNewFrameCallback* mNewFrameCallback;
};

}; 

#endif
