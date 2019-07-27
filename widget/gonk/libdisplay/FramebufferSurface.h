















#ifndef ANDROID_SF_FRAMEBUFFER_SURFACE_H
#define ANDROID_SF_FRAMEBUFFER_SURFACE_H

#include <stdint.h>
#include <sys/types.h>

#include "DisplaySurface.h"


namespace android {


class Rect;
class String8;



class FramebufferSurface : public DisplaySurface {
public:
    FramebufferSurface(int disp, uint32_t width, uint32_t height, uint32_t format, const sp<StreamConsumer>& sc);

    
    virtual status_t beginFrame(bool mustRecompose);
    virtual status_t prepareFrame(CompositionType compositionType);
    virtual status_t compositionComplete();
    virtual status_t advanceFrame();
    virtual void onFrameCommitted();
    
    
    virtual void dump(String8& result) const;
    
    
    virtual void resizeBuffers(const uint32_t , const uint32_t ) { };

    
    
    
    
    
    
    status_t setReleaseFenceFd(int fenceFd);

    virtual int GetPrevDispAcquireFd();

private:
    virtual ~FramebufferSurface() { }; 

#if ANDROID_VERSION >= 22
    virtual void onFrameAvailable(const ::android::BufferItem &item);
#else
    virtual void onFrameAvailable();
#endif
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    status_t nextBuffer(sp<GraphicBuffer>& outBuffer, sp<Fence>& outFence);

    
    int mDisplayType;

    
    
    
    int mCurrentBufferSlot;

    
    
    sp<GraphicBuffer> mCurrentBuffer;

    android::sp<android::Fence> mPrevFBAcquireFence;
};


}; 


#endif 

