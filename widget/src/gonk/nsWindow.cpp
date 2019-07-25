




































#include "nsAutoPtr.h"
#include "nsAppShell.h"
#include "nsTArray.h"
#include "nsWindow.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "ui/FramebufferNativeWindow.h"

#include "LayerManagerOGL.h"
#include "GLContextProvider.h"

#include "android/log.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gonk" , ## args)

#define IS_TOPLEVEL() (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog)

using namespace mozilla::gl;
using namespace mozilla::layers;

nsIntRect gScreenBounds;

static nsRefPtr<GLContext> sGLContext;
static nsTArray<nsWindow *> sTopWindows;
static nsWindow *gWindowToRedraw = nsnull;
static nsWindow *gFocusedWindow = nsnull;
static android::FramebufferNativeWindow *gNativeWindow = nsnull;

nsWindow::nsWindow()
{
    if (!sGLContext) {
        gNativeWindow = new android::FramebufferNativeWindow();
        sGLContext = GLContextProvider::CreateForWindow(this);
        
    }
}

nsWindow::~nsWindow()
{
}

void
nsWindow::DoDraw(void)
{
    if (!gWindowToRedraw)
        return;

    nsPaintEvent event(true, NS_PAINT, gWindowToRedraw);
    event.region = gScreenBounds;
    static_cast<LayerManagerOGL*>(gWindowToRedraw->GetLayerManager(nsnull))->
                    SetClippingRegion(nsIntRegion(gScreenBounds));
    gWindowToRedraw->mEventCallback(&event);
}

nsEventStatus
nsWindow::DispatchInputEvent(nsGUIEvent &aEvent)
{
    if (!gFocusedWindow)
        return nsEventStatus_eIgnore;

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
    if (!IS_TOPLEVEL())
        return NS_OK;

    if (aState)
        BringToTop();
    else
        mVisible = false;

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
nsWindow::Invalidate(const nsIntRect &aRect,
                     bool aIsSynchronous)
{
    gWindowToRedraw = this;
    gDrawRequest = true;
    mozilla::NotifyEvent();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Update()
{
    Invalidate(gScreenBounds, false);
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
    return NS_OK;
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

    if (!sGLContext) {
        LOG(" -- no GLContext\n");
        return nsnull;
    }

    nsRefPtr<LayerManagerOGL> layerManager =
        new LayerManagerOGL(this);

    if (layerManager->Initialize(sGLContext))
        mLayerManager = layerManager;
    else
        LOG("Could not create LayerManager");

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
}

