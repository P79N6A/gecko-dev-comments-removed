















#ifndef ANDROID_GUI_CONSUMERBASE_H
#define ANDROID_GUI_CONSUMERBASE_H

#include <gui/BufferQueue.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

namespace android {


class String8;




class ConsumerBase : public virtual RefBase,
        protected BufferQueue::ConsumerListener {
public:
    struct FrameAvailableListener : public virtual RefBase {
        
        
        
        
        
        
        
        
        virtual void onFrameAvailable() = 0;
    };

    virtual ~ConsumerBase();

    
    
    
    
    
    
    
    
    
    void abandon();

    
    
    void setName(const String8& name);

    
    
    sp<BufferQueue> getBufferQueue() const;

    
    
    
    void dump(String8& result) const;
    void dump(String8& result, const char* prefix, char* buffer, size_t SIZE) const;

    
    
    void setFrameAvailableListener(const wp<FrameAvailableListener>& listener);

private:
    ConsumerBase(const ConsumerBase&);
    void operator=(const ConsumerBase&);

protected:

    
    
    ConsumerBase(const sp<BufferQueue> &bufferQueue);

    
    
    
    
    
    
    
    
    
    
    virtual void onLastStrongRef(const void* id);

    
    
    
    
    
    virtual void onFrameAvailable();
    virtual void onBuffersReleased();

    
    
    
    
    
    
    
    
    
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    
    
    
    
    
    
    
    virtual void abandonLocked();

    
    
    
    
    
    
    
    
    
    
    
    virtual void dumpLocked(String8& result, const char* prefix, char* buffer,
            size_t size) const;

    
    
    
    
    
    
    
    virtual status_t acquireBufferLocked(BufferQueue::BufferItem *item);

    
    
    
    
    
    
    
    virtual status_t releaseBufferLocked(int buf, EGLDisplay display,
           EGLSyncKHR eglFence);

    
    
    
    
    
    status_t addReleaseFence(int slot, const sp<Fence>& fence);
    status_t addReleaseFenceLocked(int slot, const sp<Fence>& fence);

    
    
    struct Slot {
        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        
        
        
        sp<Fence> mFence;
    };

    
    
    
    
    
    
    
    Slot mSlots[BufferQueue::NUM_BUFFER_SLOTS];

    
    
    
    
    
    bool mAbandoned;

    
    
    String8 mName;

    
    
    
    wp<FrameAvailableListener> mFrameAvailableListener;

    
    
    sp<BufferQueue> mBufferQueue;

    
    
    
    
    
    
    mutable Mutex mMutex;
};


}; 

#endif 
