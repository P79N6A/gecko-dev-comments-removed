















#ifndef ANDROID_GUI_CONSUMERBASE_H
#define ANDROID_GUI_CONSUMERBASE_H

#include <gui/BufferQueue.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include <gui/IConsumerListener.h>

namespace android {


class String8;




class ConsumerBase : public virtual RefBase,
        protected ConsumerListener {
public:
    struct FrameAvailableListener : public virtual RefBase {
        
        
        
        
        
        
        
        
        virtual void onFrameAvailable() = 0;
    };

    virtual ~ConsumerBase();

    
    
    
    
    
    
    
    
    
    void abandon();

    
    
    void setName(const String8& name);

    
    
    
    void dump(String8& result) const;
    void dump(String8& result, const char* prefix) const;

    
    
    void setFrameAvailableListener(const wp<FrameAvailableListener>& listener);

private:
    ConsumerBase(const ConsumerBase&);
    void operator=(const ConsumerBase&);

protected:
    
    
    
    
    ConsumerBase(const sp<IGraphicBufferConsumer>& consumer, bool controlledByApp = false);

    
    
    
    
    
    
    
    
    
    
    virtual void onLastStrongRef(const void* id);

    
    
    
    
    
    
    
    virtual void onFrameAvailable();
    virtual void onBuffersReleased();
    virtual void onSidebandStreamChanged();

    
    
    
    
    
    
    
    
    
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    
    
    
    
    
    
    
    virtual void abandonLocked();

    
    
    
    
    
    
    
    
    
    
    
    virtual void dumpLocked(String8& result, const char* prefix) const;

    
    
    
    
    
    
    
    virtual status_t acquireBufferLocked(IGraphicBufferConsumer::BufferItem *item,
        nsecs_t presentWhen);

    
    
    
    
    
    
    
    virtual status_t releaseBufferLocked(int slot,
            const sp<GraphicBuffer> graphicBuffer,
            EGLDisplay display, EGLSyncKHR eglFence);

    
    bool stillTracking(int slot, const sp<GraphicBuffer> graphicBuffer);

    
    
    
    
    
    status_t addReleaseFence(int slot,
            const sp<GraphicBuffer> graphicBuffer, const sp<Fence>& fence);
    status_t addReleaseFenceLocked(int slot,
            const sp<GraphicBuffer> graphicBuffer, const sp<Fence>& fence);

    
    
    struct Slot {
        
        
        sp<GraphicBuffer> mGraphicBuffer;

        
        
        
        
        sp<Fence> mFence;

        
        uint64_t mFrameNumber;
    };

    
    
    
    
    
    
    
    Slot mSlots[BufferQueue::NUM_BUFFER_SLOTS];

    
    
    
    
    
    bool mAbandoned;

    
    
    String8 mName;

    
    
    
    wp<FrameAvailableListener> mFrameAvailableListener;

    
    
    sp<IGraphicBufferConsumer> mConsumer;

    
    
    
    
    
    
    mutable Mutex mMutex;
};


}; 

#endif 
