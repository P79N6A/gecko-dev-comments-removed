














#include "mozilla/DebugOnly.h"

#include <fcntl.h>

#include "android/log.h"

#include "mozilla/dom/TabParent.h"
#include "mozilla/Hal.h"
#include "mozilla/Preferences.h"
#include "mozilla/ProcessPriorityManager.h"
#include "mozilla/Services.h"
#include "mozilla/FileUtils.h"
#include "mozilla/ClearOnShutdown.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "GLContextProvider.h"
#include "GLContext.h"
#include "GLContextEGL.h"
#include "nsAutoPtr.h"
#include "nsAppShell.h"
#include "nsIdleService.h"
#include "nsIObserverService.h"
#include "nsScreenManagerGonk.h"
#include "nsTArray.h"
#include "nsWindow.h"
#include "nsIWidgetListener.h"
#include "cutils/properties.h"
#include "ClientLayerManager.h"
#include "BasicLayers.h"
#include "libdisplay/GonkDisplay.h"
#include "pixelflinger/format.h"
#include "mozilla/TextEvents.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Logging.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/APZThreadUtils.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/InputAPZContext.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TouchEvents.h"
#include "nsThreadUtils.h"
#include "HwcComposer2D.h"
#include "VsyncSource.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)
#define LOGW(args...) __android_log_print(ANDROID_LOG_WARN, "Gonk", ## args)
#define LOGE(args...) __android_log_print(ANDROID_LOG_ERROR, "Gonk", ## args)

#define IS_TOPLEVEL() (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog)

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;
using namespace mozilla::gfx;
using namespace mozilla::gl;
using namespace mozilla::layers;
using namespace mozilla::widget;

static nsWindow *gFocusedWindow = nullptr;

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

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)

nsWindow::nsWindow()
{
    mFramebuffer = nullptr;

    nsRefPtr<nsScreenManagerGonk> screenManager = nsScreenManagerGonk::GetInstance();
    screenManager->Initialize();

    
    
    
    
    
    
    gfxPlatform::GetPlatform();
    if (!ShouldUseOffMainThreadCompositing()) {
        MOZ_CRASH("How can we render apps, then?");
    }
}

nsWindow::~nsWindow()
{
    if (mScreen->IsPrimaryScreen()) {
        HwcComposer2D::GetInstance()->SetCompositorParent(nullptr);
    }
}

void
nsWindow::DoDraw(void)
{
    if (!hal::GetScreenEnabled()) {
        gDrawRequest = true;
        return;
    }

    nsRefPtr<nsScreenGonk> screen = nsScreenManagerGonk::GetPrimaryScreen();
    const nsTArray<nsWindow*>& windows = screen->GetTopWindows();

    if (windows.IsEmpty()) {
        LOG("  no window to draw, bailing");
        return;
    }

    nsWindow *targetWindow = (nsWindow *)windows[0];
    while (targetWindow->GetLastChild()) {
        targetWindow = (nsWindow *)targetWindow->GetLastChild();
    }

    nsIWidgetListener* listener = targetWindow->GetWidgetListener();
    if (listener) {
        listener->WillPaintWindow(targetWindow);
    }

    LayerManager* lm = targetWindow->GetLayerManager();
    if (mozilla::layers::LayersBackend::LAYERS_CLIENT == lm->GetBackendType()) {
        
    } else {
        NS_RUNTIMEABORT("Unexpected layer manager type");
    }

    listener = targetWindow->GetWidgetListener();
    if (listener) {
        listener->DidPaintWindow();
    }
}

void
nsWindow::ConfigureAPZControllerThread()
{
    APZThreadUtils::SetControllerThread(CompositorParent::CompositorLoop());
}

 nsEventStatus
nsWindow::DispatchKeyInput(WidgetKeyboardEvent& aEvent)
{
    if (!gFocusedWindow) {
        return nsEventStatus_eIgnore;
    }

    gFocusedWindow->UserActivity();

    nsEventStatus status;
    aEvent.widget = gFocusedWindow;
    gFocusedWindow->DispatchEvent(&aEvent, status);
    return status;
}

 void
nsWindow::DispatchTouchInput(MultiTouchInput& aInput)
{
    APZThreadUtils::AssertOnControllerThread();

    if (!gFocusedWindow) {
        return;
    }

    gFocusedWindow->DispatchTouchInputViaAPZ(aInput);
}

class DispatchTouchInputOnMainThread : public nsRunnable
{
public:
    DispatchTouchInputOnMainThread(const MultiTouchInput& aInput,
                                   const ScrollableLayerGuid& aGuid,
                                   const uint64_t& aInputBlockId,
                                   nsEventStatus aApzResponse)
      : mInput(aInput)
      , mGuid(aGuid)
      , mInputBlockId(aInputBlockId)
      , mApzResponse(aApzResponse)
    {}

    NS_IMETHOD Run() {
        if (gFocusedWindow) {
            gFocusedWindow->DispatchTouchEventForAPZ(mInput, mGuid, mInputBlockId, mApzResponse);
        }
        return NS_OK;
    }

private:
    MultiTouchInput mInput;
    ScrollableLayerGuid mGuid;
    uint64_t mInputBlockId;
    nsEventStatus mApzResponse;
};

void
nsWindow::DispatchTouchInputViaAPZ(MultiTouchInput& aInput)
{
    APZThreadUtils::AssertOnControllerThread();

    if (!mAPZC) {
        
        
        return;
    }

    
    mozilla::layers::ScrollableLayerGuid guid;
    uint64_t inputBlockId;
    nsEventStatus result = mAPZC->ReceiveInputEvent(aInput, &guid, &inputBlockId);
    
    if (result == nsEventStatus_eConsumeNoDefault) {
        return;
    }

    
    
    
    
    NS_DispatchToMainThread(new DispatchTouchInputOnMainThread(
        aInput, guid, inputBlockId, result));
}

void
nsWindow::DispatchTouchEventForAPZ(const MultiTouchInput& aInput,
                                   const ScrollableLayerGuid& aGuid,
                                   const uint64_t aInputBlockId,
                                   nsEventStatus aApzResponse)
{
    MOZ_ASSERT(NS_IsMainThread());
    UserActivity();

    
    WidgetTouchEvent event = aInput.ToWidgetTouchEvent(this);

    
    
    
    
    ProcessUntransformedAPZEvent(&event, aGuid, aInputBlockId, aApzResponse);
}

class DispatchTouchInputOnControllerThread : public Task
{
public:
    DispatchTouchInputOnControllerThread(const MultiTouchInput& aInput)
      : Task()
      , mInput(aInput)
    {}

    virtual void Run() override {
        if (gFocusedWindow) {
            gFocusedWindow->DispatchTouchInputViaAPZ(mInput);
        }
    }

private:
    MultiTouchInput mInput;
};

nsresult
nsWindow::SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                     TouchPointerState aPointerState,
                                     nsIntPoint aPointerScreenPoint,
                                     double aPointerPressure,
                                     uint32_t aPointerOrientation,
                                     nsIObserver* aObserver)
{
    AutoObserverNotifier notifier(aObserver, "touchpoint");

    if (aPointerState == TOUCH_HOVER) {
        return NS_ERROR_UNEXPECTED;
    }

    if (!mSynthesizedTouchInput) {
        mSynthesizedTouchInput = new MultiTouchInput();
    }

    
    
    
    
    
    MultiTouchInput inputToDispatch;
    inputToDispatch.mInputType = MULTITOUCH_INPUT;

    int32_t index = mSynthesizedTouchInput->IndexOfTouch((int32_t)aPointerId);
    if (aPointerState == TOUCH_CONTACT) {
        if (index >= 0) {
            
            SingleTouchData& point = mSynthesizedTouchInput->mTouches[index];
            point.mScreenPoint = ScreenIntPoint::FromUntyped(aPointerScreenPoint);
            point.mRotationAngle = (float)aPointerOrientation;
            point.mForce = (float)aPointerPressure;
            inputToDispatch.mType = MultiTouchInput::MULTITOUCH_MOVE;
        } else {
            
            mSynthesizedTouchInput->mTouches.AppendElement(SingleTouchData(
                (int32_t)aPointerId,
                ScreenIntPoint::FromUntyped(aPointerScreenPoint),
                ScreenSize(0, 0),
                (float)aPointerOrientation,
                (float)aPointerPressure));
            inputToDispatch.mType = MultiTouchInput::MULTITOUCH_START;
        }
        inputToDispatch.mTouches = mSynthesizedTouchInput->mTouches;
    } else {
        MOZ_ASSERT(aPointerState == TOUCH_REMOVE || aPointerState == TOUCH_CANCEL);
        
        if (index >= 0) {
            mSynthesizedTouchInput->mTouches.RemoveElementAt(index);
        }
        inputToDispatch.mType = (aPointerState == TOUCH_REMOVE
            ? MultiTouchInput::MULTITOUCH_END
            : MultiTouchInput::MULTITOUCH_CANCEL);
        inputToDispatch.mTouches.AppendElement(SingleTouchData(
            (int32_t)aPointerId,
            ScreenIntPoint::FromUntyped(aPointerScreenPoint),
            ScreenSize(0, 0),
            (float)aPointerOrientation,
            (float)aPointerPressure));
    }

    
    
    
    
    
    
    APZThreadUtils::RunOnControllerThread(new DispatchTouchInputOnControllerThread(inputToDispatch));

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Create(nsIWidget *aParent,
                 void *aNativeParent,
                 const nsIntRect &aRect,
                 nsWidgetInitData *aInitData)
{
    BaseCreate(aParent, aRect, aInitData);

    nsCOMPtr<nsIScreen> screen;

    uint32_t screenId = aParent ? ((nsWindow*)aParent)->mScreen->GetId() :
                                  aInitData->mScreenId;

    nsRefPtr<nsScreenManagerGonk> screenManager = nsScreenManagerGonk::GetInstance();
    screenManager->ScreenForId(screenId, getter_AddRefs(screen));

    mScreen = static_cast<nsScreenGonk*>(screen.get());

    mBounds = aRect;

    mParent = (nsWindow *)aParent;
    mVisible = false;

    if (!aParent) {
        mBounds = mScreen->GetRect();
    }

    if (!IS_TOPLEVEL()) {
        return NS_OK;
    }

    mScreen->RegisterWindow(this);

    Resize(0, 0, mBounds.width, mBounds.height, false);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    mOnDestroyCalled = true;
    mScreen->UnregisterWindow(this);
    if (this == gFocusedWindow) {
        gFocusedWindow = nullptr;
    }
    nsBaseWidget::OnDestroy();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Show(bool aState)
{
    if (mWindowType == eWindowType_invisible) {
        return NS_OK;
    }

    if (mVisible == aState) {
        return NS_OK;
    }

    mVisible = aState;
    if (!IS_TOPLEVEL()) {
        return mParent ? mParent->Show(aState) : NS_OK;
    }

    if (aState) {
        BringToTop();
    } else {
        const nsTArray<nsWindow*>& windows =
            mScreen->GetTopWindows();
        for (unsigned int i = 0; i < windows.Length(); i++) {
            nsWindow *win = windows[i];
            if (!win->mVisible) {
                continue;
            }
            win->BringToTop();
            break;
        }
    }

    return NS_OK;
}

bool
nsWindow::IsVisible() const
{
    return mVisible;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(bool aAllowSlop,
                            int32_t *aX,
                            int32_t *aY)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Move(double aX,
               double aY)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aWidth,
                 double aHeight,
                 bool   aRepaint)
{
    return Resize(0, 0, aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP
nsWindow::Resize(double aX,
                 double aY,
                 double aWidth,
                 double aHeight,
                 bool   aRepaint)
{
    mBounds = nsIntRect(NSToIntRound(aX), NSToIntRound(aY),
                        NSToIntRound(aWidth), NSToIntRound(aHeight));
    if (mWidgetListener) {
        mWidgetListener->WindowResized(this, mBounds.width, mBounds.height);
    }

    if (aRepaint) {
        Invalidate(mBounds);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Enable(bool aState)
{
    return NS_OK;
}

bool
nsWindow::IsEnabled() const
{
    return true;
}

NS_IMETHODIMP
nsWindow::SetFocus(bool aRaise)
{
    if (aRaise) {
        BringToTop();
    }

    if (!IS_TOPLEVEL() && mScreen->IsPrimaryScreen()) {
        
        gFocusedWindow = this;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConfigureChildren(const nsTArray<nsIWidget::Configuration>&)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsIntRect &aRect)
{
    nsWindow *top = mParent;
    while (top && top->mParent) {
        top = top->mParent;
    }
    const nsTArray<nsWindow*>& windows = mScreen->GetTopWindows();
    if (top != windows[0] && this != windows[0]) {
        return NS_OK;
    }

    gDrawRequest = true;
    mozilla::NotifyEvent();
    return NS_OK;
}

LayoutDeviceIntPoint
nsWindow::WidgetToScreenOffset()
{
    LayoutDeviceIntPoint p(0, 0);
    nsWindow *w = this;

    while (w && w->mParent) {
        p.x += w->mBounds.x;
        p.y += w->mBounds.y;

        w = w->mParent;
    }

    return p;
}

void*
nsWindow::GetNativeData(uint32_t aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
        
        return mScreen->GetNativeWindow();
    }
    return nullptr;
}

void
nsWindow::SetNativeData(uint32_t aDataType, uintptr_t aVal)
{
    switch (aDataType) {
    case NS_NATIVE_OPENGL_CONTEXT:
        
        GLContext* context = reinterpret_cast<GLContext*>(aVal);

        HwcComposer2D* hwc = HwcComposer2D::GetInstance();
        hwc->SetEGLInfo(GLContextEGL::Cast(context)->GetEGLDisplay(),
                        GLContextEGL::Cast(context)->GetEGLSurface(),
                        context);
        return;
    }
}

NS_IMETHODIMP
nsWindow::DispatchEvent(WidgetGUIEvent* aEvent, nsEventStatus& aStatus)
{
    if (mWidgetListener) {
      aStatus = mWidgetListener->HandleEvent(aEvent, mUseAttachedEvents);
    }
    return NS_OK;
}

NS_IMETHODIMP_(void)
nsWindow::SetInputContext(const InputContext& aContext,
                          const InputContextAction& aAction)
{
    mInputContext = aContext;
}

NS_IMETHODIMP_(InputContext)
nsWindow::GetInputContext()
{
    
    mInputContext.mNativeIMEContext = nullptr;
    return mInputContext;
}

NS_IMETHODIMP
nsWindow::ReparentNativeWidget(nsIWidget* aNewParent)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(bool aFullScreen, nsIScreen*)
{
    if (mWindowType != eWindowType_toplevel) {
        
        NS_WARNING("MakeFullScreen() on a dialog or child widget?");
        return nsBaseWidget::MakeFullScreen(aFullScreen);
    }

    if (aFullScreen) {
        
        
        
        
        
        nsIntRect virtualBounds;
        mScreen->GetRect(&virtualBounds.x, &virtualBounds.y,
                         &virtualBounds.width, &virtualBounds.height);
        Resize(virtualBounds.x, virtualBounds.y,
               virtualBounds.width, virtualBounds.height,
               true);
    }
    return NS_OK;
}

static gralloc_module_t const*
gralloc_module()
{
    hw_module_t const *module;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module)) {
        return nullptr;
    }
    return reinterpret_cast<gralloc_module_t const*>(module);
}

static SurfaceFormat
HalFormatToSurfaceFormat(int aHalFormat, int* bytepp)
{
    switch (aHalFormat) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        *bytepp = 4;
        return SurfaceFormat::R8G8B8A8;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        *bytepp = 4;
        return SurfaceFormat::R8G8B8X8;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        *bytepp = 4;
        return SurfaceFormat::B8G8R8A8;
    case HAL_PIXEL_FORMAT_RGB_565:
        *bytepp = 2;
        return SurfaceFormat::R5G6B5;
    default:
        MOZ_CRASH("Unhandled HAL pixel format");
        return SurfaceFormat::UNKNOWN; 
    }
}

TemporaryRef<DrawTarget>
nsWindow::StartRemoteDrawing()
{
    GonkDisplay* display = GetGonkDisplay();
    mFramebuffer = display->DequeueBuffer();
    int width = mFramebuffer->width, height = mFramebuffer->height;
    void *vaddr;
    if (gralloc_module()->lock(gralloc_module(), mFramebuffer->handle,
                               GRALLOC_USAGE_SW_READ_NEVER |
                               GRALLOC_USAGE_SW_WRITE_OFTEN |
                               GRALLOC_USAGE_HW_FB,
                               0, 0, width, height, &vaddr)) {
        EndRemoteDrawing();
        return nullptr;
    }
    int bytepp;
    SurfaceFormat format = HalFormatToSurfaceFormat(display->surfaceformat,
                                                    &bytepp);
    mFramebufferTarget = Factory::CreateDrawTargetForData(
         BackendType::CAIRO, (uint8_t*)vaddr,
         IntSize(width, height), mFramebuffer->stride * bytepp, format);
    if (!mFramebufferTarget) {
        MOZ_CRASH("nsWindow::StartRemoteDrawing failed in CreateDrawTargetForData");
    }
    if (!mBackBuffer ||
        mBackBuffer->GetSize() != mFramebufferTarget->GetSize() ||
        mBackBuffer->GetFormat() != mFramebufferTarget->GetFormat()) {
        mBackBuffer = mFramebufferTarget->CreateSimilarDrawTarget(
            mFramebufferTarget->GetSize(), mFramebufferTarget->GetFormat());
    }
    RefPtr<DrawTarget> buffer(mBackBuffer);
    return buffer.forget();
}

void
nsWindow::EndRemoteDrawing()
{
    if (mFramebufferTarget) {
        IntSize size = mFramebufferTarget->GetSize();
        Rect rect(0, 0, size.width, size.height);
        RefPtr<SourceSurface> source = mBackBuffer->Snapshot();
        mFramebufferTarget->DrawSurface(source, rect, rect);
        gralloc_module()->unlock(gralloc_module(), mFramebuffer->handle);
    }
    if (mFramebuffer) {
        GetGonkDisplay()->QueueBuffer(mFramebuffer);
    }
    mFramebuffer = nullptr;
    mFramebufferTarget = nullptr;
}

float
nsWindow::GetDPI()
{
    return mScreen->GetDpi();
}

double
nsWindow::GetDefaultScaleInternal()
{
    float dpi = GetDPI();
    
    
    
    if (dpi < 200.0) {
        return 1.0; 
    }
    if (dpi < 300.0) {
        return 1.5; 
    }
    
    return floor(dpi / 150.0 + 0.5);
}

LayerManager *
nsWindow::GetLayerManager(PLayerTransactionChild* aShadowManager,
                          LayersBackend aBackendHint,
                          LayerManagerPersistence aPersistence,
                          bool* aAllowRetaining)
{
    if (aAllowRetaining) {
        *aAllowRetaining = true;
    }
    if (mLayerManager) {
        
        
        if (mLayerManager->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
            ClientLayerManager* manager =
                static_cast<ClientLayerManager*>(mLayerManager.get());
            uint32_t rotation = mScreen->EffectiveScreenRotation();
            manager->SetDefaultTargetConfiguration(mozilla::layers::BufferMode::BUFFER_NONE,
                                                   ScreenRotation(rotation));
        }
        return mLayerManager;
    }

    
    
    mUseLayersAcceleration = ComputeShouldAccelerate(mUseLayersAcceleration);

    const nsTArray<nsWindow*>& windows = mScreen->GetTopWindows();
    nsWindow *topWindow = windows[0];

    if (!topWindow) {
        LOGW(" -- no topwindow\n");
        return nullptr;
    }

    CreateCompositor();
    if (mCompositorParent && mScreen->IsPrimaryScreen()) {
        HwcComposer2D::GetInstance()->SetCompositorParent(mCompositorParent);
    }
    MOZ_ASSERT(mLayerManager);
    return mLayerManager;
}

void
nsWindow::BringToTop()
{
    const nsTArray<nsWindow*>& windows = mScreen->GetTopWindows();
    if (!windows.IsEmpty()) {
        if (nsIWidgetListener* listener = windows[0]->GetWidgetListener()) {
            listener->WindowDeactivated();
        }
    }

    mScreen->BringToTop(this);

    if (mWidgetListener) {
        mWidgetListener->WindowActivated();
    }

    Invalidate(mBounds);
}

void
nsWindow::UserActivity()
{
    if (!mIdleService) {
        mIdleService = do_GetService("@mozilla.org/widget/idleservice;1");
    }

    if (mIdleService) {
        mIdleService->ResetIdleTimeOut(0);
    }
}

uint32_t
nsWindow::GetGLFrameBufferFormat()
{
    if (mLayerManager &&
        mLayerManager->GetBackendType() == mozilla::layers::LayersBackend::LAYERS_OPENGL) {
        
        
        return LOCAL_GL_RGB;
    }
    return LOCAL_GL_NONE;
}

nsIntRect
nsWindow::GetNaturalBounds()
{
    return mScreen->GetNaturalBounds();
}

bool
nsWindow::NeedsPaint()
{
  if (!mLayerManager) {
    return false;
  }
  return nsIWidget::NeedsPaint();
}

Composer2D*
nsWindow::GetComposer2D()
{
    if (!mScreen->IsPrimaryScreen()) {
        return nullptr;
    }

    return HwcComposer2D::GetInstance();
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
