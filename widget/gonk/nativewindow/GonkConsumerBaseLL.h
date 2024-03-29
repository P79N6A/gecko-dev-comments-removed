
















#ifndef NATIVEWINDOW_GONKCONSUMERBASE_LL_H
#define NATIVEWINDOW_GONKCONSUMERBASE_LL_H

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include <gui/IConsumerListener.h>

#include "GonkBufferQueueLL.h"

namespace android {


class String8;




class GonkConsumerBase : public virtual RefBase,
        protected ConsumerListener {
public:
    struct FrameAvailableListener : public virtual RefBase {
        
        
        
        
        
        
        
        
        virtual void onFrameAvailable() = 0;
    };

    virtual ~GonkConsumerBase();

    
    
    
    
    
    
    
    
    
    void abandon();

    
    
    void setName(const String8& name);

    
    
    
    void dump(String8& result) const;
    void dump(String8& result, const char* prefix) const;

    
    
    void setFrameAvailableListener(const wp<FrameAvailableListener>& listener);

private:
    GonkConsumerBase(const GonkConsumerBase&);
    void operator=(const GonkConsumerBase&);

protected:
    
    
    
    
    GonkConsumerBase(const sp<IGonkGraphicBufferConsumer>& consumer, bool controlledByApp = false);

    
    
    
    
    
    
    
    
    
    
    virtual void onLastStrongRef(const void* id);

    
    
    
    
    
    
    
#if ANDROID_VERSION == 21
    virtual void onFrameAvailable();
#else
    virtual void onFrameAvailable(const ::android::BufferItem& item);
    virtual void onFrameReplaced(const ::android::BufferItem& item) {};
#endif
    virtual void onBuffersReleased();
    virtual void onSidebandStreamChanged();

    
    
    
    
    
    
    
    
    
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    
    
    
    
    
    
    
    virtual void abandonLocked();

    
    
    
    
    
    
    
    
    
    
    
    virtual void dumpLocked(String8& result, const char* prefix) const;

    
    
    
    
    
    
    
    virtual status_t acquireBufferLocked(IGonkGraphicBufferConsumer::BufferItem *item,
        nsecs_t presentWhen);

    
    
    
    
    
    
    
    virtual status_t releaseBufferLocked(int slot, const sp<GraphicBuffer> graphicBuffer);

    
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

    
    
    
    
    
    
    
    Slot mSlots[GonkBufferQueue::NUM_BUFFER_SLOTS];

    
    
    
    
    
    bool mAbandoned;

    
    
    String8 mName;

    
    
    
    wp<FrameAvailableListener> mFrameAvailableListener;

    
    
    sp<IGonkGraphicBufferConsumer> mConsumer;

    
    
    
    
    
    
    mutable Mutex mMutex;
};


}; 

#endif 
