
















#ifndef NATIVEWINDOW_GONKBUFFERSLOT_LL_H
#define NATIVEWINDOW_GONKBUFFERSLOT_LL_H

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <utils/StrongPointer.h>

#include "mozilla/layers/TextureClient.h"

namespace android {

struct GonkBufferSlot {
    typedef mozilla::layers::TextureClient TextureClient;

    GonkBufferSlot()
    : mBufferState(GonkBufferSlot::FREE),
      mRequestBufferCalled(false),
      mFrameNumber(0),
      mAcquireCalled(false),
      mNeedsCleanupOnRelease(false),
      mAttachedByConsumer(false) {
    }

    
    
    sp<GraphicBuffer> mGraphicBuffer;

    
    
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

    
    
    
    
    
    
    
    
    
    sp<Fence> mFence;

    
    bool mAcquireCalled;

    
    
    
    bool mNeedsCleanupOnRelease;

    
    
    
    bool mAttachedByConsumer;

    
    mozilla::RefPtr<TextureClient> mTextureClient;
};

} 

#endif
