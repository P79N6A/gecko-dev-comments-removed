















#ifndef ANDROID_SF_FRAMEBUFFER_SURFACE_H
#define ANDROID_SF_FRAMEBUFFER_SURFACE_H

#include <stdint.h>
#include <sys/types.h>

#include <gui/ConsumerBase.h>


namespace android {


class Rect;
class String8;
class HWComposer;



class FramebufferSurface : public ConsumerBase {
public:
    FramebufferSurface(int disp, uint32_t width, uint32_t height, uint32_t format, sp<IGraphicBufferAlloc>& alloc);

    bool isUpdateOnDemand() const { return false; }
    status_t setUpdateRectangle(const Rect& updateRect);
    status_t compositionComplete();

    virtual void dump(String8& result);

    
    
    
    
    
    
    status_t setReleaseFenceFd(int fenceFd);

    buffer_handle_t lastHandle;
    int lastFenceFD;
private:
    virtual ~FramebufferSurface() { }; 

    virtual void onFrameAvailable();
    virtual void freeBufferLocked(int slotIndex);

    
    
    
    status_t nextBuffer(sp<GraphicBuffer>& outBuffer, sp<Fence>& outFence);

    
    int mDisplayType;

    
    
    
    int mCurrentBufferSlot;

    
    
    sp<GraphicBuffer> mCurrentBuffer;
};


}; 


#endif 

