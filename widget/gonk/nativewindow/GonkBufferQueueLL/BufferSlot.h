















#ifndef ANDROID_GUI_BUFFERSLOT_H
#define ANDROID_GUI_BUFFERSLOT_H

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <utils/StrongPointer.h>

namespace android {

class Fence;

struct BufferSlot {

    BufferSlot()
    : mEglDisplay(EGL_NO_DISPLAY),
      mBufferState(BufferSlot::FREE),
      mRequestBufferCalled(false),
      mFrameNumber(0),
      mEglFence(EGL_NO_SYNC_KHR),
      mAcquireCalled(false),
      mNeedsCleanupOnRelease(false),
      mAttachedByConsumer(false) {
    }

    
    
    sp<GraphicBuffer> mGraphicBuffer;

    
    EGLDisplay mEglDisplay;

    
    
    enum BufferState {
        
        
        
        
        
        
        
        FREE = 0,

        
        
        
        
        
        
        
        DEQUEUED = 1,

        
        
        
        
        
        
        
        
        
        QUEUED = 2,

        
        
        
        
        
        
        ACQUIRED = 3
    };

    static const char* bufferStateName(BufferState state);

    
    BufferState mBufferState;

    
    
    
    bool mRequestBufferCalled;

    
    
    
    uint64_t mFrameNumber;

    
    
    
    
    
    EGLSyncKHR mEglFence;

    
    
    
    
    
    
    
    
    
    sp<Fence> mFence;

    
    bool mAcquireCalled;

    
    
    
    bool mNeedsCleanupOnRelease;

    
    
    
    bool mAttachedByConsumer;
};

} 

#endif
