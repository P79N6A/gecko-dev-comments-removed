
















#ifndef NATIVEWINDOW_GONKCONSUMERBASE_H
#define NATIVEWINDOW_GONKCONSUMERBASE_H

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#include "GonkBufferQueue.h"

namespace android {


class String8;




class GonkConsumerBase : public virtual RefBase,
        protected GonkBufferQueue::ConsumerListener {
public:
    struct FrameAvailableListener : public virtual RefBase {
        
        
        
        
        
        
        
        
        virtual void onFrameAvailable() = 0;
    };

    virtual ~GonkConsumerBase();

    
    
    
    
    
    
    
    
    
    void abandon();

    
    
    void setName(const String8& name);

    
    
    sp<GonkBufferQueue> getBufferQueue() const;

    
    
    
    void dump(String8& result) const;
    void dump(String8& result, const char* prefix, char* buffer, size_t SIZE) const;

    
    
#if ANDROID_VERSION == 17
    void setFrameAvailableListener(const sp<FrameAvailableListener>& listener);
#else
    void setFrameAvailableListener(const wp<FrameAvailableListener>& listener);
#endif

private:
    GonkConsumerBase(const GonkConsumerBase&);
    void operator=(const GonkConsumerBase&);

protected:

    
    
    GonkConsumerBase(const sp<GonkBufferQueue> &bufferQueue);

    
    
    
    
    
    
    
    
    
    
    virtual void onLastStrongRef(const void* id);

    
    
    
    
    
    virtual void onFrameAvailable();
    virtual void onBuffersReleased();

    
    
    
    
    
    
    
    
    
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    
    
    
    
    
    
    
    virtual void abandonLocked();

    
    
    
    
    
    
    
    
    
    
    
    virtual void dumpLocked(String8& result, const char* prefix, char* buffer,
            size_t size) const;

    
    
    
    
    
    
    
    virtual status_t acquireBufferLocked(GonkBufferQueue::BufferItem *item);

    
    
    
    
    
    
    
    virtual status_t releaseBufferLocked(int buf);

    
    
    
    
    
    status_t addReleaseFence(int slot, const sp<Fence>& fence);
    status_t addReleaseFenceLocked(int slot, const sp<Fence>& fence);

    
    
    struct Slot {
        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        
        
        
        sp<Fence> mFence;
    };

    
    
    
    
    
    
    
    Slot mSlots[GonkBufferQueue::NUM_BUFFER_SLOTS];

    
    
    
    
    
    bool mAbandoned;

    
    
    String8 mName;

    
    
    
#if ANDROID_VERSION == 17
    sp<FrameAvailableListener> mFrameAvailableListener;
#else
    wp<FrameAvailableListener> mFrameAvailableListener;
#endif

    
    
    sp<GonkBufferQueue> mBufferQueue;

    
    
    
    
    
    
    mutable Mutex mMutex;
};


}; 

#endif 
