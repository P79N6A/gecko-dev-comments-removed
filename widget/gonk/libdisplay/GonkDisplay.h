














#ifndef GONKDISPLAY_H
#define GONKDISPLAY_H

#include <system/window.h>
#include <utils/StrongPointer.h>
#include "mozilla/Types.h"

namespace android {
class DisplaySurface;
class IGraphicBufferProducer;
}

namespace mozilla {

typedef void * EGLDisplay;
typedef void * EGLSurface;

class MOZ_EXPORT GonkDisplay {
public:
   







    enum DisplayType {
        DISPLAY_PRIMARY,
        DISPLAY_EXTERNAL,
        DISPLAY_VIRTUAL,
        NUM_DISPLAY_TYPES
    };

    struct NativeData {
        android::sp<ANativeWindow> mNativeWindow;
#if ANDROID_VERSION >= 17
        android::sp<android::DisplaySurface> mDisplaySurface;
#endif
        float mXdpi;
    };

    virtual void SetEnabled(bool enabled) = 0;

    typedef void (*OnEnabledCallbackType)(bool enabled);

    virtual void OnEnabled(OnEnabledCallbackType callback) = 0;

    virtual void* GetHWCDevice() = 0;

    


    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur) = 0;

    virtual ANativeWindowBuffer* DequeueBuffer() = 0;

    virtual bool QueueBuffer(ANativeWindowBuffer* buf) = 0;

    virtual void UpdateDispSurface(EGLDisplay dpy, EGLSurface sur) = 0;

    virtual NativeData GetNativeData(
        GonkDisplay::DisplayType aDisplayType,
        android::IGraphicBufferProducer* aProducer = nullptr) = 0;

    virtual void NotifyBootAnimationStopped() = 0;

    float xdpi;
    int32_t surfaceformat;
};

MOZ_EXPORT __attribute__ ((weak))
GonkDisplay* GetGonkDisplay();

}
#endif
