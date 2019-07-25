




































#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "android/log.h"
#include "ui/FramebufferNativeWindow.h"

#include "mozilla/Hal.h"
#include "Framebuffer.h"
#include "gfxContext.h"
#include "gfxUtils.h"
#include "GLContextProvider.h"
#include "LayerManagerOGL.h"
#include "nsAutoPtr.h"
#include "nsAppShell.h"
#include "nsIdleService.h"
#include "nsScreenManagerGonk.h"
#include "nsTArray.h"
#include "nsWindow.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)

#define IS_TOPLEVEL() (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog)

using namespace mozilla;
using namespace mozilla::gl;
using namespace mozilla::layers;
using namespace mozilla::widget;

nsIntRect gScreenBounds;

static nsRefPtr<GLContext> sGLContext;
static nsTArray<nsWindow *> sTopWindows;
static nsWindow *gWindowToRedraw = nsnull;
static nsWindow *gFocusedWindow = nsnull;
static android::FramebufferNativeWindow *gNativeWindow = nsnull;
static bool sFramebufferOpen;

nsWindow::nsWindow()
{
    if (!sGLContext && !sFramebufferOpen) {
        
        
        gNativeWindow = new android::FramebufferNativeWindow();
        sGLContext = GLContextProvider::CreateForWindow(this);
        
        if (!sGLContext) {
            LOG("Failed to create GL context for fb, trying /dev/graphics/fb0");

            

            nsIntSize screenSize;
            sFramebufferOpen = Framebuffer::Open(&screenSize);
            gScreenBounds = nsIntRect(nsIntPoint(0, 0), screenSize);
            if (!sFramebufferOpen) {
                LOG("Failed to mmap fb(?!?), aborting ...");
                NS_RUNTIMEABORT("Can't open GL context and can't fall back on /dev/graphics/fb0 ...");
            }
        }
    }
}

nsWindow::~nsWindow()
{
}

void
nsWindow::DoDraw(void)
{
    if (!hal::GetScreenEnabled()) {
        gDrawRequest = true;
        return;
    }

    if (!gWindowToRedraw) {
        LOG("  no window to draw, bailing");
        return;
    }

    nsPaintEvent event(true, NS_PAINT, gWindowToRedraw);
    event.region = gWindowToRedraw->mDirtyRegion;
    gWindowToRedraw->mDirtyRegion.SetEmpty();

    LayerManager* lm = gWindowToRedraw->GetLayerManager();
    if (LayerManager::LAYERS_OPENGL == lm->GetBackendType()) {
        static_cast<LayerManagerOGL*>(lm)->SetClippingRegion(event.region);
        gWindowToRedraw->mEventCallback(&event);
    } else if (LayerManager::LAYERS_BASIC == lm->GetBackendType()) {
        MOZ_ASSERT(sFramebufferOpen);

        nsRefPtr<gfxASurface> backBuffer = Framebuffer::BackBuffer();
        {
            nsRefPtr<gfxContext> ctx = new gfxContext(backBuffer);
            gfxUtils::PathFromRegion(ctx, event.region);
            ctx->Clip();

            
            AutoLayerManagerSetup setupLayerManager(
                gWindowToRedraw, ctx, BasicLayerManager::BUFFER_NONE);
            gWindowToRedraw->mEventCallback(&event);
        }
        backBuffer->Flush();

        Framebuffer::Present(event.region);
    } else {
        NS_RUNTIMEABORT("Unexpected layer manager type");
    }
}

nsEventStatus
nsWindow::DispatchInputEvent(nsGUIEvent &aEvent)
{
    if (!gFocusedWindow)
        return nsEventStatus_eIgnore;

    gFocusedWindow->UserActivity();
    aEvent.widget = gFocusedWindow;
    return gFocusedWindow->mEventCallback(&aEvent);
}

NS_IMETHODIMP
nsWindow::Create(nsIWidget *aParent,
                 void *aNativeParent,
                 const nsIntRect &aRect,
                 EVENT_CALLBACK aHandleEventFunction,
                 nsDeviceContext *aContext,
                 nsWidgetInitData *aInitData)
{
    BaseCreate(aParent, IS_TOPLEVEL() ? gScreenBounds : aRect,
               aHandleEventFunction, aContext, aInitData);

    mBounds = aRect;

    nsWindow *parent = (nsWindow *)aNativeParent;
    mParent = parent;

    if (!aNativeParent) {
        mBounds = gScreenBounds;
    }

    if (!IS_TOPLEVEL())
        return NS_OK;

    sTopWindows.AppendElement(this);

    Resize(0, 0, gScreenBounds.width, gScreenBounds.height, false);
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    sTopWindows.RemoveElement(this);
    if (this == gWindowToRedraw)
        gWindowToRedraw = nsnull;
    if (this == gFocusedWindow)
        gFocusedWindow = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Show(bool aState)
{
    if (mWindowType == eWindowType_invisible)
        return NS_OK;

    if (mVisible == aState)
        return NS_OK;

    mVisible = aState;
    if (!IS_TOPLEVEL())
        return mParent ? mParent->Show(aState) : NS_OK;

    if (aState) {
        BringToTop();
    } else {
        for (unsigned int i = 0; i < sTopWindows.Length(); i++) {
            nsWindow *win = sTopWindows[i];
            if (!win->mVisible)
                continue;

            win->BringToTop();
            break;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsVisible(bool & aState)
{
    aState = mVisible;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(bool aAllowSlop,
                            PRInt32 *aX,
                            PRInt32 *aY)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Move(PRInt32 aX,
               PRInt32 aY)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(PRInt32 aWidth,
                 PRInt32 aHeight,
                 bool    aRepaint)
{
    return Resize(0, 0, aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP
nsWindow::Resize(PRInt32 aX,
                 PRInt32 aY,
                 PRInt32 aWidth,
                 PRInt32 aHeight,
                 bool    aRepaint)
{
    nsSizeEvent event(true, NS_SIZE, this);
    event.time = PR_Now() / 1000;

    nsIntRect rect(aX, aY, aWidth, aHeight);
    event.windowSize = &rect;
    event.mWinWidth = gScreenBounds.width;
    event.mWinHeight = gScreenBounds.height;

    (*mEventCallback)(&event);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Enable(bool aState)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsEnabled(bool *aState)
{
    *aState = true;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetFocus(bool aRaise)
{
    if (aRaise)
        BringToTop();

    gFocusedWindow = this;
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
    nsWindow *parent = mParent;
    while (parent && parent != sTopWindows[0])
        parent = parent->mParent;
    if (parent != sTopWindows[0])
        return NS_OK;

    mDirtyRegion.Or(mDirtyRegion, aRect);
    gWindowToRedraw = this;
    gDrawRequest = true;
    mozilla::NotifyEvent();
    return NS_OK;
}

nsIntPoint
nsWindow::WidgetToScreenOffset()
{
    nsIntPoint p(0, 0);
    nsWindow *w = this;

    while (w && w->mParent) {
        p.x += w->mBounds.x;
        p.y += w->mBounds.y;

        w = w->mParent;
    }

    return p;
}

void*
nsWindow::GetNativeData(PRUint32 aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
        return gNativeWindow;
    case NS_NATIVE_WIDGET:
        return this;
    }
    return nsnull;
}

NS_IMETHODIMP
nsWindow::DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus)
{
    aStatus = (*mEventCallback)(aEvent);
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
    return mInputContext;
}

NS_IMETHODIMP
nsWindow::ReparentNativeWidget(nsIWidget* aNewParent)
{
    return NS_OK;
}

float
nsWindow::GetDPI()
{
    return gNativeWindow->xdpi;
}

LayerManager *
nsWindow::GetLayerManager(PLayersChild* aShadowManager,
                          LayersBackend aBackendHint,
                          LayerManagerPersistence aPersistence,
                          bool* aAllowRetaining)
{
    if (aAllowRetaining)
        *aAllowRetaining = true;
    if (mLayerManager)
        return mLayerManager;

    nsWindow *topWindow = sTopWindows[0];

    if (!topWindow) {
        LOG(" -- no topwindow\n");
        return nsnull;
    }

    if (sGLContext) {
        nsRefPtr<LayerManagerOGL> layerManager = new LayerManagerOGL(this);

        if (layerManager->Initialize(sGLContext))
            mLayerManager = layerManager;
        else
            LOG("Could not create OGL LayerManager");
    } else {
        MOZ_ASSERT(sFramebufferOpen);
        mLayerManager = new BasicShadowLayerManager(this);
    }

    return mLayerManager;
}

gfxASurface *
nsWindow::GetThebesSurface()
{
    



    
    
    return new gfxImageSurface(gfxIntSize(5,5), gfxImageSurface::ImageFormatRGB24);
}

void
nsWindow::BringToTop()
{
    if (!sTopWindows.IsEmpty()) {
        nsGUIEvent event(true, NS_DEACTIVATE, sTopWindows[0]);
        (*mEventCallback)(&event);
    }

    sTopWindows.RemoveElement(this);
    sTopWindows.InsertElementAt(0, this);

    nsGUIEvent event(true, NS_ACTIVATE, this);
    (*mEventCallback)(&event);
    Invalidate(gScreenBounds);
}

void
nsWindow::UserActivity()
{
    if (!mIdleService) {
        mIdleService = do_GetService("@mozilla.org/widget/idleservice;1");
    }

    if (mIdleService) {
        mIdleService->ResetIdleTimeOut();
    }
}

PRUint32
nsWindow::GetGLFrameBufferFormat()
{
    if (mLayerManager &&
        mLayerManager->GetBackendType() == LayerManager::LAYERS_OPENGL) {
        
        
        return LOCAL_GL_RGB;
    }
    return LOCAL_GL_NONE;
}



nsScreenGonk::nsScreenGonk(void *nativeScreen)
{
}

nsScreenGonk::~nsScreenGonk()
{
}

NS_IMETHODIMP
nsScreenGonk::GetRect(PRInt32 *outLeft,  PRInt32 *outTop,
                      PRInt32 *outWidth, PRInt32 *outHeight)
{
    *outLeft = gScreenBounds.x;
    *outTop = gScreenBounds.y;

    *outWidth = gScreenBounds.width;
    *outHeight = gScreenBounds.height;

    return NS_OK;
}

NS_IMETHODIMP
nsScreenGonk::GetAvailRect(PRInt32 *outLeft,  PRInt32 *outTop,
                           PRInt32 *outWidth, PRInt32 *outHeight)
{
    return GetRect(outLeft, outTop, outWidth, outHeight);
}


NS_IMETHODIMP
nsScreenGonk::GetPixelDepth(PRInt32 *aPixelDepth)
{
    
    
    *aPixelDepth = 24;
    return NS_OK;
}

NS_IMETHODIMP
nsScreenGonk::GetColorDepth(PRInt32 *aColorDepth)
{
    return GetPixelDepth(aColorDepth);
}

NS_IMPL_ISUPPORTS1(nsScreenManagerGonk, nsIScreenManager)

nsScreenManagerGonk::nsScreenManagerGonk()
{
    mOneScreen = new nsScreenGonk(nsnull);
}

nsScreenManagerGonk::~nsScreenManagerGonk()
{
}

NS_IMETHODIMP
nsScreenManagerGonk::GetPrimaryScreen(nsIScreen **outScreen)
{
    NS_IF_ADDREF(*outScreen = mOneScreen.get());
    return NS_OK;
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForRect(PRInt32 inLeft,
                                   PRInt32 inTop,
                                   PRInt32 inWidth,
                                   PRInt32 inHeight,
                                   nsIScreen **outScreen)
{
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerGonk::ScreenForNativeWidget(void *aWidget, nsIScreen **outScreen)
{
    return GetPrimaryScreen(outScreen);
}

NS_IMETHODIMP
nsScreenManagerGonk::GetNumberOfScreens(PRUint32 *aNumberOfScreens)
{
    *aNumberOfScreens = 1;
    return NS_OK;
}
