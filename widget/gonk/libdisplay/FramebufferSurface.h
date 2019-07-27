















#ifndef ANDROID_SF_FRAMEBUFFER_SURFACE_H
#define ANDROID_SF_FRAMEBUFFER_SURFACE_H

#include <stdint.h>
#include <sys/types.h>

#include <gui/ConsumerBase.h>


namespace android {


class Rect;
class String8;

#if ANDROID_VERSION >= 21
typedef IGraphicBufferConsumer StreamConsumer;
#else
typedef BufferQueue StreamConsumer;
#endif



class FramebufferSurface : public ConsumerBase {
public:
    FramebufferSurface(int disp, uint32_t width, uint32_t height, uint32_t format, const sp<StreamConsumer>& sc);

    bool isUpdateOnDemand() const { return false; }
    status_t setUpdateRectangle(const Rect& updateRect);
    status_t compositionComplete();

    virtual void dump(String8& result);
    virtual void dump(String8& result, const char* prefix);

    
    
    
    
    
    
    status_t setReleaseFenceFd(int fenceFd);

    virtual int GetPrevFBAcquireFd();

    buffer_handle_t lastHandle;
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

