















#ifndef nsScreenManagerGonk_h___
#define nsScreenManagerGonk_h___

#include "mozilla/Hal.h"

#include "cutils/properties.h"
#include "hardware/hwcomposer.h"
#include "libdisplay/GonkDisplay.h"
#include "nsBaseScreen.h"
#include "nsCOMPtr.h"
#include "nsIScreenManager.h"

class nsRunnable;
class nsWindow;

namespace android {
    class DisplaySurface;
    class IGraphicBufferProducer;
};

namespace mozilla {
namespace gl {
    class GLContext;
}
}

class nsScreenGonk : public nsBaseScreen
{
    typedef mozilla::hal::ScreenConfiguration ScreenConfiguration;
    typedef mozilla::GonkDisplay GonkDisplay;

public:
    nsScreenGonk(uint32_t aId,
                 GonkDisplay::DisplayType aDisplayType,
                 const GonkDisplay::NativeData& aNativeData);

    ~nsScreenGonk();

    NS_IMETHOD GetId(uint32_t* aId);
    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight);
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth);
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth);
    NS_IMETHOD GetRotation(uint32_t* aRotation);
    NS_IMETHOD SetRotation(uint32_t  aRotation);

    uint32_t GetId();
    nsIntRect GetRect();
    float GetDpi();
    int32_t GetSurfaceFormat();
    ANativeWindow* GetNativeWindow();
    nsIntRect GetNaturalBounds();
    uint32_t EffectiveScreenRotation();
    ScreenConfiguration GetConfiguration();
    bool IsPrimaryScreen();

#if ANDROID_VERSION >= 17
    android::DisplaySurface* GetDisplaySurface();
    int GetPrevDispAcquireFd();
#endif
    GonkDisplay::DisplayType GetDisplayType();

    void RegisterWindow(nsWindow* aWindow);
    void UnregisterWindow(nsWindow* aWindow);
    void BringToTop(nsWindow* aWindow);

    const nsTArray<nsWindow*>& GetTopWindows() const
    {
        return mTopWindows;
    }

    
    void SetEGLInfo(hwc_display_t aDisplay, hwc_surface_t aSurface,
                    mozilla::gl::GLContext* aGLContext);
    hwc_display_t GetDpy();
    hwc_surface_t GetSur();

protected:
    uint32_t mId;
    int32_t mColorDepth;
    android::sp<ANativeWindow> mNativeWindow;
    float mDpi;
    int32_t mSurfaceFormat;
    nsIntRect mNaturalBounds; 
    nsIntRect mVirtualBounds; 
    uint32_t mScreenRotation;
    uint32_t mPhysicalScreenRotation;
    nsTArray<nsWindow*> mTopWindows;
#if ANDROID_VERSION >= 17
    android::sp<android::DisplaySurface> mDisplaySurface;
#endif
    GonkDisplay::DisplayType mDisplayType;
    hwc_display_t mDpy; 
    hwc_surface_t mSur; 
    mozilla::gl::GLContext* mGLContext; 
};

class nsScreenManagerGonk final : public nsIScreenManager
{
public:
    typedef mozilla::GonkDisplay GonkDisplay;

public:
    nsScreenManagerGonk();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREENMANAGER

    static already_AddRefed<nsScreenManagerGonk> GetInstance();
    static already_AddRefed<nsScreenGonk> GetPrimaryScreen();

    void Initialize();
    void DisplayEnabled(bool aEnabled);

    nsresult AddScreen(GonkDisplay::DisplayType aDisplayType,
                       android::IGraphicBufferProducer* aProducer = nullptr);

    nsresult RemoveScreen(GonkDisplay::DisplayType aDisplayType);

protected:
    ~nsScreenManagerGonk();
    void VsyncControl(bool aEnabled);
    uint32_t GetIdFromType(GonkDisplay::DisplayType aDisplayType);
    bool IsScreenConnected(uint32_t aId);

    bool mInitialized;
    nsTArray<nsRefPtr<nsScreenGonk>> mScreens;
    nsRefPtr<nsRunnable> mScreenOnEvent;
    nsRefPtr<nsRunnable> mScreenOffEvent;
};

#endif 
