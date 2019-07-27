
















#ifndef NATIVEWINDOW_FAKE_SURFACE_COMPOSER_H
#define NATIVEWINDOW_FAKE_SURFACE_COMPOSER_H

#include <stdint.h>
#include <sys/types.h>

#include <utils/Errors.h>

#include <binder/BinderService.h>

#include <gui/ISurfaceComposer.h>
#include <gui/ISurfaceComposerClient.h>

namespace android {



class IGraphicBufferAlloc;

class FakeSurfaceComposer : public BinderService<FakeSurfaceComposer>,
                            public BnSurfaceComposer
{
public:
    static char const* getServiceName() {
        return "FakeSurfaceComposer";
    }

    
    
    static  void instantiate();

#if ANDROID_VERSION >= 19
    virtual void destroyDisplay(const sp<android::IBinder>& display);
#endif

#if ANDROID_VERSION >= 21
    virtual status_t captureScreen(const sp<IBinder>& display,
                                   const sp<IGraphicBufferProducer>& producer,
                                   Rect sourceCrop, uint32_t reqWidth, uint32_t reqHeight,
                                   uint32_t minLayerZ, uint32_t maxLayerZ,
                                   bool useIdentityTransform,
                                   Rotation rotation = eRotateNone);
#elif ANDROID_VERSION >= 19
    virtual status_t captureScreen(const sp<IBinder>& display,
                                   const sp<IGraphicBufferProducer>& producer,
                                   uint32_t reqWidth, uint32_t reqHeight,
                                   uint32_t minLayerZ, uint32_t maxLayerZ);
#else
    virtual status_t captureScreen(const sp<IBinder>& display,
                                   const sp<IGraphicBufferProducer>& producer,
                                   uint32_t reqWidth, uint32_t reqHeight,
                                   uint32_t minLayerZ, uint32_t maxLayerZ, bool isCpuConsumer);
#endif

private:
    FakeSurfaceComposer();
    
    virtual ~FakeSurfaceComposer();

    


    virtual sp<ISurfaceComposerClient> createConnection();
    virtual sp<IGraphicBufferAlloc> createGraphicBufferAlloc();
    virtual sp<IBinder> createDisplay(const String8& displayName, bool secure);
    virtual sp<IBinder> getBuiltInDisplay(int32_t id);
    virtual void setTransactionState(const Vector<ComposerState>& state,
            const Vector<DisplayState>& displays, uint32_t flags);
    virtual void bootFinished();
    virtual bool authenticateSurfaceTexture(
        const sp<IGraphicBufferProducer>& bufferProducer) const;
    virtual sp<IDisplayEventConnection> createDisplayEventConnection();
#if ANDROID_VERSION >= 21
    virtual void setPowerMode(const sp<IBinder>& display, int mode);
    virtual status_t getDisplayConfigs(const sp<IBinder>& display, Vector<DisplayInfo>* configs);
    virtual status_t getDisplayStats(const sp<IBinder>& display, DisplayStatInfo* stats);
    virtual int getActiveConfig(const sp<IBinder>& display);
    virtual status_t setActiveConfig(const sp<IBinder>& display, int id);
    virtual status_t clearAnimationFrameStats();
    virtual status_t getAnimationFrameStats(FrameStats* outStats) const;
#elif ANDROID_VERSION >= 17
    
    virtual void blank(const sp<IBinder>& display);
    
    virtual void unblank(const sp<IBinder>& display);
    virtual status_t getDisplayInfo(const sp<IBinder>& display, DisplayInfo* info);
#endif

};


}; 

#endif 
