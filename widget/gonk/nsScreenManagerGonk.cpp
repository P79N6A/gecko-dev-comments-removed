














#include "android/log.h"
#include "GLContext.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/Hal.h"
#include "libdisplay/GonkDisplay.h"
#include "libdisplay/DisplaySurface.h"
#include "nsScreenManagerGonk.h"
#include "nsThreadUtils.h"
#include "HwcComposer2D.h"
#include "VsyncSource.h"
#include "nsWindow.h"
#include "mozilla/Services.h"
#include "mozilla/ProcessPriorityManager.h"
#include "nsIdleService.h"
#include "nsIObserverService.h"
#include "nsAppShell.h"
#include "nsTArray.h"
#include "pixelflinger/format.h"
#include "cutils/properties.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "nsScreenGonk" , ## args)
#define LOGW(args...) __android_log_print(ANDROID_LOG_WARN, "nsScreenGonk", ## args)
#define LOGE(args...) __android_log_print(ANDROID_LOG_ERROR, "nsScreenGonk", ## args)

using namespace mozilla;
using namespace mozilla::hal;
using namespace mozilla::gfx;
using namespace mozilla::gl;
using namespace mozilla::dom;

namespace {

class ScreenOnOffEvent : public nsRunnable {
public:
    ScreenOnOffEvent(bool on)
        : mIsOn(on)
    {}

    NS_IMETHOD Run() {
        
        nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
        if (observerService) {
          observerService->NotifyObservers(
            nullptr, "screen-state-changed",
            mIsOn ? MOZ_UTF16("on") : MOZ_UTF16("off")
          );
        }

        nsRefPtr<nsScreenGonk> screen = nsScreenManagerGonk::GetPrimaryScreen();
        const nsTArray<nsWindow*>& windows = screen->GetTopWindows();

        for (uint32_t i = 0; i < windows.Length(); i++) {
            nsWindow *win = windows[i];

            if (nsIWidgetListener* listener = win->GetWidgetListener()) {
                listener->SizeModeChanged(mIsOn ? nsSizeMode_Fullscreen : nsSizeMode_Minimized);
            }
        }

        return NS_OK;
    }

private:
    bool mIsOn;
};

static void
displayEnabledCallback(bool enabled)
{
    nsRefPtr<nsScreenManagerGonk> screenManager = nsScreenManagerGonk::GetInstance();
    screenManager->DisplayEnabled(enabled);
}

} 

static uint32_t
SurfaceFormatToColorDepth(int32_t aSurfaceFormat)
{
    switch (aSurfaceFormat) {
    case GGL_PIXEL_FORMAT_RGB_565:
        return 16;
    case GGL_PIXEL_FORMAT_RGBA_8888:
        return 32;
    }
    return 24; 
}



nsScreenGonk::nsScreenGonk(uint32_t aId, ANativeWindow* aNativeWindow)
    : mId(aId)
    , mNativeWindow(aNativeWindow)
    , mScreenRotation(nsIScreen::ROTATION_0_DEG)
    , mPhysicalScreenRotation(nsIScreen::ROTATION_0_DEG)
{
    int surfaceFormat;
    if (mNativeWindow->query(mNativeWindow.get(), NATIVE_WINDOW_WIDTH, &mVirtualBounds.width) ||
        mNativeWindow->query(mNativeWindow.get(), NATIVE_WINDOW_HEIGHT, &mVirtualBounds.height) ||
        mNativeWindow->query(mNativeWindow.get(), NATIVE_WINDOW_FORMAT, &surfaceFormat)) {
        NS_RUNTIMEABORT("Failed to get native window size, aborting...");
    }

    mNaturalBounds = mVirtualBounds;

    if (IsPrimaryScreen()) {
        char propValue[PROPERTY_VALUE_MAX];
        property_get("ro.sf.hwrotation", propValue, "0");
        mPhysicalScreenRotation = atoi(propValue) / 90;
    }

    mDpi = GetGonkDisplay()->xdpi;
    mColorDepth = SurfaceFormatToColorDepth(surfaceFormat);
}

nsScreenGonk::~nsScreenGonk()
{
}

bool
nsScreenGonk::IsPrimaryScreen()
{
    return nsScreenManagerGonk::PRIMARY_SCREEN_ID == mId;
}

NS_IMETHODIMP
nsScreenGonk::GetId(uint32_t *outId)
{
    *outId = mId;
    return NS_OK;
}

uint32_t
nsScreenGonk::GetId()
{
    return mId;
}

NS_IMETHODIMP
nsScreenGonk::GetRect(int32_t *outLeft,  int32_t *outTop,
                      int32_t *outWidth, int32_t *outHeight)
{
    *outLeft = mVirtualBounds.x;
    *outTop = mVirtualBounds.y;

    *outWidth = mVirtualBounds.width;
    *outHeight = mVirtualBounds.height;

    return NS_OK;
}

nsIntRect
nsScreenGonk::GetRect()
{
    return mVirtualBounds;
}

NS_IMETHODIMP
nsScreenGonk::GetAvailRect(int32_t *outLeft,  int32_t *outTop,
                           int32_t *outWidth, int32_t *outHeight)
{
    return GetRect(outLeft, outTop, outWidth, outHeight);
}

NS_IMETHODIMP
nsScreenGonk::GetPixelDepth(int32_t *aPixelDepth)
{
    
    
    *aPixelDepth = mColorDepth;
    return NS_OK;
}

NS_IMETHODIMP
nsScreenGonk::GetColorDepth(int32_t *aColorDepth)
{
    *aColorDepth = mColorDepth;
    return NS_OK;
}

NS_IMETHODIMP
nsScreenGonk::GetRotation(uint32_t* aRotation)
{
    *aRotation = mScreenRotation;
    return NS_OK;
}

float
nsScreenGonk::GetDpi()
{
    return mDpi;
}

ANativeWindow*
nsScreenGonk::GetNativeWindow()
{
    return mNativeWindow.get();
}

NS_IMETHODIMP
nsScreenGonk::SetRotation(uint32_t aRotation)
{
    if (!(aRotation <= ROTATION_270_DEG)) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    if (mScreenRotation == aRotation) {
        return NS_OK;
    }

    mScreenRotation = aRotation;
    uint32_t rotation = EffectiveScreenRotation();
    if (rotation == nsIScreen::ROTATION_90_DEG ||
        rotation == nsIScreen::ROTATION_270_DEG) {
        mVirtualBounds = nsIntRect(0, 0,
                                   mNaturalBounds.height,
                                   mNaturalBounds.width);
    } else {
        mVirtualBounds = mNaturalBounds;
    }

    nsAppShell::NotifyScreenRotation();

    for (unsigned int i = 0; i < mTopWindows.Length(); i++) {
        mTopWindows[i]->Resize(mVirtualBounds.width,
                            mVirtualBounds.height,
                            true);
    }

    return NS_OK;
}

nsIntRect
nsScreenGonk::GetNaturalBounds()
{
    return mNaturalBounds;
}

uint32_t
nsScreenGonk::EffectiveScreenRotation()
{
    return (mScreenRotation + mPhysicalScreenRotation) % (360 / 90);
}



static ScreenOrientation
ComputeOrientation(uint32_t aRotation, const nsIntSize& aScreenSize)
{
    bool naturallyPortrait = (aScreenSize.height > aScreenSize.width);
    switch (aRotation) {
    case nsIScreen::ROTATION_0_DEG:
        return (naturallyPortrait ? eScreenOrientation_PortraitPrimary :
                eScreenOrientation_LandscapePrimary);
    case nsIScreen::ROTATION_90_DEG:
        
        
        return (naturallyPortrait ? eScreenOrientation_LandscapePrimary :
                eScreenOrientation_PortraitPrimary);
    case nsIScreen::ROTATION_180_DEG:
        return (naturallyPortrait ? eScreenOrientation_PortraitSecondary :
                eScreenOrientation_LandscapeSecondary);
    case nsIScreen::ROTATION_270_DEG:
        return (naturallyPortrait ? eScreenOrientation_LandscapeSecondary :
                eScreenOrientation_PortraitSecondary);
    default:
        MOZ_CRASH("Gonk screen must always have a known rotation");
    }
}

ScreenConfiguration
nsScreenGonk::GetConfiguration()
{
    ScreenOrientation orientation = ComputeOrientation(mScreenRotation,
                                                       mNaturalBounds.Size());

    
    
    return ScreenConfiguration(mVirtualBounds, orientation,
                               mColorDepth, mColorDepth);
}

void
nsScreenGonk::RegisterWindow(nsWindow* aWindow)
{
    mTopWindows.AppendElement(aWindow);
}

void
nsScreenGonk::UnregisterWindow(nsWindow* aWindow)
{
    mTopWindows.RemoveElement(aWindow);
}

void
nsScreenGonk::BringToTop(nsWindow* aWindow)
{
    mTopWindows.RemoveElement(aWindow);
    mTopWindows.InsertElementAt(0, aWindow);
}

NS_IMPL_ISUPPORTS(nsScreenManagerGonk, nsIScreenManager)

nsScreenManagerGonk::nsScreenManagerGonk()
    : mInitialized(false)
{
}

nsScreenManagerGonk::~nsScreenManagerGonk()
{
}

 already_AddRefed<nsScreenManagerGonk>
nsScreenManagerGonk::GetInstance()
{
    nsCOMPtr<nsIScreenManager> manager;
    manager = do_GetService("@mozilla.org/gfx/screenmanager;1");
    MOZ_ASSERT(manager);
    return already_AddRefed<nsScreenManagerGonk>(
        static_cast<nsScreenManagerGonk*>(manager.forget().take()));
}

 already_AddRefed< nsScreenGonk>
nsScreenManagerGonk::GetPrimaryScreen()
{
    nsRefPtr<nsScreenManagerGonk> manager = nsScreenManagerGonk::GetInstance();
    nsCOMPtr<nsIScreen> screen;
    manager->GetPrimaryScreen(getter_AddRefs(screen));
    MOZ_ASSERT(screen);
    return already_AddRefed<nsScreenGonk>(
        static_cast<nsScreenGonk*>(screen.forget().take()));
}

void
nsScreenManagerGonk::Initialize()
{
    if (mInitialized) {
        return;
    }

    mScreenOnEvent = new ScreenOnOffEvent(true);
    mScreenOffEvent = new ScreenOnOffEvent(false);
    GetGonkDisplay()->OnEnabled(displayEnabledCallback);

    AddScreen(PRIMARY_SCREEN_TYPE);

    nsAppShell::NotifyScreenInitialized();
    mInitialized = true;
}

void
nsScreenManagerGonk::DisplayEnabled(bool aEnabled)
{
    if (gfxPrefs::HardwareVsyncEnabled()) {
        VsyncControl(aEnabled);
    }

    NS_DispatchToMainThread(aEnabled ? mScreenOnEvent : mScreenOffEvent);
}

NS_IMETHODIMP
nsScreenManagerGonk::GetPrimaryScreen(nsIScreen **outScreen)
{
    NS_IF_ADDREF(*outScreen = mScreens[0].get());
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForId(uint32_t aId,
                                 nsIScreen **outScreen)
{
    for (size_t i = 0; i < mScreens.Length(); i++) {
        if (mScreens[i]->GetId() == aId) {
            NS_IF_ADDREF(*outScreen = mScreens[i].get());
            return NS_OK;
        }
    }

    *outScreen = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForRect(int32_t inLeft,
                                   int32_t inTop,
                                   int32_t inWidth,
                                   int32_t inHeight,
                                   nsIScreen **outScreen)
{
    
    
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForNativeWidget(void *aWidget, nsIScreen **outScreen)
{
    for (size_t i = 0; i < mScreens.Length(); i++) {
        if (aWidget == mScreens[i]->GetNativeWindow()) {
            NS_IF_ADDREF(*outScreen = mScreens[i].get());
            return NS_OK;
        }
    }

    *outScreen = nullptr;
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGonk::GetNumberOfScreens(uint32_t *aNumberOfScreens)
{
    *aNumberOfScreens = mScreens.Length();
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGonk::GetSystemDefaultScale(float *aDefaultScale)
{
    *aDefaultScale = 1.0f;
    return NS_OK;
}

void
nsScreenManagerGonk::VsyncControl(bool aEnabled)
{
    MOZ_ASSERT(gfxPrefs::HardwareVsyncEnabled());

    if (!NS_IsMainThread()) {
        NS_DispatchToMainThread(
            NS_NewRunnableMethodWithArgs<bool>(this,
                                               &nsScreenManagerGonk::VsyncControl,
                                               aEnabled));
        return;
    }

    MOZ_ASSERT(NS_IsMainThread());
    VsyncSource::Display &display = gfxPlatform::GetPlatform()->GetHardwareVsync()->GetGlobalDisplay();
    if (aEnabled) {
        display.EnableVsync();
    } else {
        display.DisableVsync();
    }
}

uint32_t
nsScreenManagerGonk::GetIdFromType(uint32_t aDisplayType)
{
    
    

    
    return aDisplayType;
}

void
nsScreenManagerGonk::AddScreen(uint32_t aDisplayType)
{
    
    MOZ_ASSERT(PRIMARY_SCREEN_TYPE == aDisplayType);

    uint32_t id = GetIdFromType(aDisplayType);

    ANativeWindow* win = GetGonkDisplay()->GetNativeWindow();
    nsScreenGonk* screen = new nsScreenGonk(id, win);

    mScreens.AppendElement(screen);
}

void
nsScreenManagerGonk::RemoveScreen(uint32_t aDisplayType)
{
    uint32_t screenId = GetIdFromType(aDisplayType);
    for (size_t i = 0; i < mScreens.Length(); i++) {
        if (mScreens[i]->GetId() == screenId) {
            mScreens.RemoveElementAt(i);
            break;
        }
    }
}
