















#ifndef ANDROID_SF_DISPLAY_SURFACE_H
#define ANDROID_SF_DISPLAY_SURFACE_H

#include <gui/ConsumerBase.h>
#include <system/window.h>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>


namespace android {


class IGraphicBufferProducer;
class String8;

#if ANDROID_VERSION >= 21
typedef IGraphicBufferConsumer StreamConsumer;
#else
typedef BufferQueue StreamConsumer;
#endif

class DisplaySurface : public ConsumerBase {
public:
    
    
    
    
    
    virtual status_t beginFrame(bool mustRecompose) = 0;

    
    
    
    
    enum CompositionType {
        COMPOSITION_UNKNOWN = 0,
        COMPOSITION_GLES    = 1,
        COMPOSITION_HWC     = 2,
        COMPOSITION_MIXED   = COMPOSITION_GLES | COMPOSITION_HWC
    };
    virtual status_t prepareFrame(CompositionType compositionType) = 0;

    
    
    
    
    virtual status_t compositionComplete() = 0;

    
    
    
    
    
    
    
    
    virtual status_t advanceFrame() = 0;

    
    
    
    virtual void onFrameCommitted() = 0;

    virtual void dump(String8& result) const = 0;

    virtual void resizeBuffers(const uint32_t w, const uint32_t h) = 0;

    
    
    
    
    
    
    virtual status_t setReleaseFenceFd(int fenceFd) = 0;

    virtual int GetPrevDispAcquireFd() = 0;

    buffer_handle_t lastHandle;

protected:
    DisplaySurface(const sp<StreamConsumer>& sc)
#if ANDROID_VERSION >= 19
        : ConsumerBase(sc, true)
#else
        : ConsumerBase(sc)
#endif
        , lastHandle(0)
    { }
    virtual ~DisplaySurface() {}
};


} 


#endif 

