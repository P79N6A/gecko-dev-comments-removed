








































#include "prlink.h"

#include <qevent.h> 
#include <QtGui>
#include <qcursor.h>

#include "nsWindow.h"
#include "nsToolkit.h"
#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"
#include "nsIRegion.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIDOMNode.h"

#include "nsWidgetsCID.h"
#include "nsIDragService.h"

#include "nsQtKeyUtils.h"

#include <X11/XF86keysym.h>

#include "nsWidgetAtoms.h"

#ifdef MOZ_ENABLE_STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <startup-notification-1.0/libsn/sn.h>
#endif

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsGfxCIID.h"


#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"
#include "nsILocalFile.h"


#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsGfxCIID.h"
#include "nsIImage.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"

#include "gfxPlatformQt.h"
#include "gfxXlibSurface.h"
#include "gfxQPainterSurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qwidget.h>
#include "qx11info_x11.h"
#include <qcursor.h>
#include <qobject.h>
#include <execinfo.h>
#include <stdlib.h>

#include <execinfo.h>

#include "mozqwidget.h"


static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);







static nsresult    initialize_prefs        (void);

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);

#define NS_WINDOW_TITLE_MAX_LENGTH 4095

#define kWindowPositionSlop 20


static const int WHEEL_DELTA = 120;
static PRBool gGlobalsInitialized = PR_FALSE;

static bool ignoreEvent(nsEventStatus aStatus)
{
    return aStatus == nsEventStatus_eConsumeNoDefault;
}

static PRBool
isContextMenuKey(const nsKeyEvent &aKeyEvent)
{
    return ((aKeyEvent.keyCode == NS_VK_F10 && aKeyEvent.isShift &&
             !aKeyEvent.isControl && !aKeyEvent.isMeta && !aKeyEvent.isAlt) ||
            (aKeyEvent.keyCode == NS_VK_CONTEXT_MENU && !aKeyEvent.isShift &&
             !aKeyEvent.isControl && !aKeyEvent.isMeta && !aKeyEvent.isAlt));
}

static void
keyEventToContextMenuEvent(const nsKeyEvent* aKeyEvent,
                           nsMouseEvent* aCMEvent)
{
    memcpy(aCMEvent, aKeyEvent, sizeof(nsInputEvent));

    aCMEvent->isShift = aCMEvent->isControl = PR_FALSE;
    aCMEvent->isControl = PR_FALSE;
    aCMEvent->isAlt = aCMEvent->isMeta = PR_FALSE;
    aCMEvent->isMeta = PR_FALSE;
    aCMEvent->clickCount = 0;
    aCMEvent->acceptActivation = PR_FALSE;
}

nsWindow::nsWindow()
{
    mDrawingarea         = nsnull;
    mIsVisible           = PR_FALSE;
    mRetryPointerGrab    = PR_FALSE;
    mRetryKeyboardGrab   = PR_FALSE;
    mActivatePending     = PR_FALSE;
    mWindowType          = eWindowType_child;
    mSizeState           = nsSizeMode_Normal;
    mPluginType          = PluginType_NONE;
    mQCursor             = Qt::ArrowCursor;

    if (!gGlobalsInitialized) {
        gGlobalsInitialized = PR_TRUE;

        
        initialize_prefs();
    }

    memset(mKeyDownFlags, 0, sizeof(mKeyDownFlags));

    mIsTransparent = PR_FALSE;
    mTransparencyBitmap = nsnull;

    mTransparencyBitmapWidth  = 0;
    mTransparencyBitmapHeight = 0;
    mCursor = eCursor_standard;
}

nsWindow::~nsWindow()
{
    LOG(("nsWindow::~nsWindow() [%p]\n", (void *)this));

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = nsnull;

    Destroy();
}

void
nsWindow::Initialize(QWidget *widget)
{
    Q_ASSERT(widget);

    mDrawingarea = widget;
    mDrawingarea->setMouseTracking(PR_TRUE);
    mDrawingarea->setFocusPolicy(Qt::WheelFocus);
}

 void
nsWindow::ReleaseGlobals()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsCommonWidget,
                             nsISupportsWeakReference)

NS_IMETHODIMP
nsWindow::Create(nsIWidget        *aParent,
                 const nsRect     &aRect,
                 EVENT_CALLBACK   aHandleEventFunction,
                 nsIDeviceContext *aContext,
                 nsIAppShell      *aAppShell,
                 nsIToolkit       *aToolkit,
                 nsWidgetInitData *aInitData)
{
    nsresult rv = NativeCreate(aParent, nsnull, aRect, aHandleEventFunction,
                               aContext, aAppShell, aToolkit, aInitData);
    return rv;
}

NS_IMETHODIMP
nsWindow::Create(nsNativeWidget aParent,
                 const nsRect     &aRect,
                 EVENT_CALLBACK   aHandleEventFunction,
                 nsIDeviceContext *aContext,
                 nsIAppShell      *aAppShell,
                 nsIToolkit       *aToolkit,
                 nsWidgetInitData *aInitData)
{
    nsresult rv = NativeCreate(nsnull, aParent, aRect, aHandleEventFunction,
                               aContext, aAppShell, aToolkit, aInitData);
    return rv;
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    if (mIsDestroyed || !mCreated)
        return NS_OK;

    LOG(("nsWindow::Destroy [%p]\n", (void *)this));
    mIsDestroyed = PR_TRUE;
    mCreated = PR_FALSE;

    NativeShow(PR_FALSE);

    
    
    
    for (nsIWidget* kid = mFirstChild; kid; ) {
        nsIWidget* next = kid->GetNextSibling();
        kid->Destroy();
        kid = next;
    }

    
    
    mThebesSurface = nsnull;

    if (mDrawingarea) {
        delete mDrawingarea;
        mDrawingarea = nsnull;
    }

    OnDestroy();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetParent(nsIWidget *aNewParent)
{
    NS_ENSURE_ARG_POINTER(aNewParent);

    QWidget* newParentWindow =
        static_cast<QWidget*>(aNewParent->GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(newParentWindow, "Parent widget has a null native window handle");

    if (mDrawingarea) {
        qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
        
    } else {
        NS_NOTREACHED("nsWindow::SetParent - reparenting a non-child window");
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetModal(PRBool aModal)
{
    LOG(("nsWindow::SetModal [%p] %d, widget[%p]\n", (void *)this, aModal, mDrawingarea));

    MozQWidget *mozWidget = static_cast<MozQWidget*>(mDrawingarea);
    if (mozWidget)
        mozWidget->setModal(aModal);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsVisible(PRBool & aState)
{
    aState = mDrawingarea?mDrawingarea->isVisible():PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY)
{
    if (mDrawingarea) {
        PRInt32 screenWidth  = QApplication::desktop()->width();
        PRInt32 screenHeight = QApplication::desktop()->height();
        if (aAllowSlop) {
            if (*aX < (kWindowPositionSlop - mBounds.width))
                *aX = kWindowPositionSlop - mBounds.width;
            if (*aX > (screenWidth - kWindowPositionSlop))
                *aX = screenWidth - kWindowPositionSlop;
            if (*aY < (kWindowPositionSlop - mBounds.height))
                *aY = kWindowPositionSlop - mBounds.height;
            if (*aY > (screenHeight - kWindowPositionSlop))
                *aY = screenHeight - kWindowPositionSlop;
        } else {
            if (*aX < 0)
                *aX = 0;
            if (*aX > (screenWidth - mBounds.width))
                *aX = screenWidth - mBounds.width;
            if (*aY < 0)
                *aY = 0;
            if (*aY > (screenHeight - mBounds.height))
                *aY = screenHeight - mBounds.height;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
    LOG(("nsWindow::Move [%p] %d %d\n", (void *)this,
         aX, aY));

    mPlaced = PR_TRUE;

    
    
    
    
    if (aX == mBounds.x && aY == mBounds.y &&
        mWindowType != eWindowType_popup)
        return NS_OK;

    


    if (!mDrawingarea)
        return NS_OK;

    QPoint pos(aX, aY);
    if (mDrawingarea) {
        if (mParent && mDrawingarea->windowType() == Qt::Popup) {
            nsRect oldrect, newrect;
            oldrect.x = aX;
            oldrect.y = aY;

            mParent->WidgetToScreen(oldrect, newrect);

            pos = QPoint(newrect.x, newrect.y);
#ifdef DEBUG_WIDGETS
            qDebug("pos is [%d,%d]", pos.x(), pos.y());
#endif
        } else {
            qDebug("Widget within another? (%p)", (void*)mDrawingarea);
        }
    }

    mBounds.x = pos.x();
    mBounds.y = pos.y();

    if (!mCreated)
        return NS_OK;

    if (mIsTopLevel) {
        mDrawingarea->move(pos);
    }
    else if (mDrawingarea) {
        mDrawingarea->move(pos);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                      nsIWidget                  *aWidget,
                      PRBool                      aActivate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetZIndex(PRInt32 aZIndex)
{
    nsIWidget* oldPrev = GetPrevSibling();

    nsBaseWidget::SetZIndex(aZIndex);

    if (GetPrevSibling() == oldPrev) {
        return NS_OK;
    }

    NS_ASSERTION(!mDrawingarea, "Expected Mozilla child widget");

    
    

    if (!GetNextSibling()) {
        
        if (mDrawingarea) {
            qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
            
        }
    } else {
        
        for (nsWindow* w = this; w;
             w = static_cast<nsWindow*>(w->GetPrevSibling())) {
            if (w->mDrawingarea) {
                qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
                
            }
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetSizeMode(PRInt32 aMode)
{
    nsresult rv;

    LOG(("nsWindow::SetSizeMode [%p] %d\n", (void *)this, aMode));

    
    rv = nsBaseWidget::SetSizeMode(aMode);

    
    
    if (!mDrawingarea || mSizeState == mSizeMode) {
        return rv;
    }

    switch (aMode) {
    case nsSizeMode_Maximized:
        mDrawingarea->showMaximized();
        break;
    case nsSizeMode_Minimized:
        mDrawingarea->showMinimized();
        break;
    default:
        
        mDrawingarea->showNormal ();
        
        
        
        
        
        break;
    }

    mSizeState = mSizeMode;

    return rv;
}

NS_IMETHODIMP
nsWindow::Enable(PRBool aState)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

typedef void (* SetUserTimeFunc)(QWidget* aWindow, quint32 aTimestamp);


































NS_IMETHODIMP
nsWindow::SetFocus(PRBool aRaise)
{
    
    

    LOGFOCUS(("  SetFocus [%p]\n", (void *)this));

    if (!mDrawingarea)
        return NS_ERROR_FAILURE;

    if (aRaise)
        mDrawingarea->raise();
    mDrawingarea->setFocus();

    
    

    LOGFOCUS(("  widget now has focus - dispatching events [%p]\n",
              (void *)this));

    DispatchGotFocusEvent();

    LOGFOCUS(("  done dispatching events in SetFocus() [%p]\n",
              (void *)this));

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetScreenBounds(nsRect &aRect)
{
    nsRect origin(0, 0, mBounds.width, mBounds.height);
    WidgetToScreen(origin, aRect);
    LOG(("GetScreenBounds %d %d | %d %d | %d %d\n",
         aRect.x, aRect.y,
         mBounds.width, mBounds.height,
         aRect.width, aRect.height));
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetForegroundColor(const nscolor &aColor)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetBackgroundColor(const nscolor &aColor)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetCursor(nsCursor aCursor)
{
    mCursor = aCursor;
    MozQWidget *mozWidget = static_cast<MozQWidget*>(mDrawingarea);
    mozWidget->SetCursor(mCursor);
    return NS_OK;
}



















































NS_IMETHODIMP
nsWindow::SetCursor(imgIContainer* aCursor,
                    PRUint32 aHotspotX, PRUint32 aHotspotY)
{
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    return rv;
}


NS_IMETHODIMP
nsWindow::Validate()
{
    
    
    if (!mDrawingarea)
        return NS_OK;

    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(PRBool aIsSynchronous)
{
    LOGDRAW(("Invalidate (all) [%p]: \n", (void *)this));

    if (!mDrawingarea)
        return NS_OK;

    if (aIsSynchronous)
        mDrawingarea->repaint();
    else
        mDrawingarea->update();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsRect &aRect,
                     PRBool        aIsSynchronous)
{
    LOGDRAW(("Invalidate (rect) [%p]: %d %d %d %d (sync: %d)\n", (void *)this,
             aRect.x, aRect.y, aRect.width, aRect.height, aIsSynchronous));

    if (!mDrawingarea)
        return NS_OK;

    if (aIsSynchronous)
        mDrawingarea->repaint(aRect.x, aRect.y, aRect.width, aRect.height);
    else
        mDrawingarea->update(aRect.x, aRect.y, aRect.width, aRect.height);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::InvalidateRegion(const nsIRegion* aRegion,
                           PRBool           aIsSynchronous)
{

    QRegion *region = nsnull;
    aRegion->GetNativeRegion((void *&)region);

    if (region && mDrawingarea) {
        QRect rect = region->boundingRect();





        if (aIsSynchronous)
            mDrawingarea->repaint(*region);
        else
            mDrawingarea->update(*region);
    }
    else {
        qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
        LOGDRAW(("Invalidate (region) [%p] with empty region\n",
                 (void *)this));
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Update()
{
    if (!mDrawingarea)
        return NS_OK;

    mDrawingarea->update();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetColorMap(nsColorMap *aColorMap)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::Scroll(PRInt32  aDx,
                 PRInt32  aDy,
                 nsRect  *aClipRect)
{
    if (!mDrawingarea)
        return NS_OK;

    mDrawingarea->scroll(aDx, aDy);

    
    for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
        nsRect bounds;
        kid->GetBounds(bounds);
        bounds.x += aDx;
        bounds.y += aDy;
        static_cast<nsBaseWidget*>(kid)->SetBounds(bounds);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScrollWidgets(PRInt32 aDx,
                        PRInt32 aDy)
{
    if (!mDrawingarea)
        return NS_OK;

    mDrawingarea->scroll(aDx, aDy);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScrollRect(nsRect  &aSrcRect,
                     PRInt32  aDx,
                     PRInt32  aDy)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

void*
nsWindow::GetNativeData(PRUint32 aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_WIDGET: {
        if (!mDrawingarea)
            return nsnull;

        return mDrawingarea;
        break;
    }

    case NS_NATIVE_PLUGIN_PORT:
        return SetupPluginPort();
        break;

    case NS_NATIVE_DISPLAY:
        return mDrawingarea->x11Info().display();
        break;

    case NS_NATIVE_GRAPHIC: {
        NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
        return (void *)static_cast<nsToolkit *>(mToolkit)->GetSharedGC();
        break;
    }

    case NS_NATIVE_SHELLWIDGET:
        return (void *) mDrawingarea;

    default:
        NS_WARNING("nsWindow::GetNativeData called with bad value");
        return nsnull;
    }
}

NS_IMETHODIMP
nsWindow::SetBorderStyle(nsBorderStyle aBorderStyle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetTitle(const nsAString& aTitle)
{
    if (!mDrawingarea)
        return NS_OK;

    nsAString::const_iterator it;
    QString qStr((QChar*)aTitle.BeginReading(it).get(), -1);

    if (mDrawingarea)
        mDrawingarea->setWindowTitle(qStr);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIcon(const nsAString& aIconSpec)
{
    if (!mDrawingarea)
        return NS_OK;

    nsCOMPtr<nsILocalFile> iconFile;
    nsCAutoString path;
    nsCStringArray iconList;

    
    
    

    const char extensions[6][7] = { ".png", "16.png", "32.png", "48.png",
                                    ".xpm", "16.xpm" };

    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(extensions); i++) {
        
        if (i == NS_ARRAY_LENGTH(extensions) - 2 && iconList.Count())
            break;

        nsAutoString extension;
        extension.AppendASCII(extensions[i]);

        ResolveIconName(aIconSpec, extension, getter_AddRefs(iconFile));
        if (iconFile) {
            iconFile->GetNativePath(path);
            iconList.AppendCString(path);
        }
    }

    
    if (iconList.Count() == 0)
        return NS_OK;

    return SetWindowIconList(iconList);
}

NS_IMETHODIMP
nsWindow::SetMenuBar(nsIMenuBar * aMenuBar)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::ShowMenuBar(PRBool aShow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
    NS_ENSURE_TRUE(mDrawingarea, NS_OK);

    PRInt32 X,Y;

    QPoint offset(0,0);
    offset = mDrawingarea->mapFromGlobal(offset);
    X = offset.x();
    Y = offset.y();
    LOG(("WidgetToScreen (container) %d %d\n", X, Y));

    aNewRect.x = aOldRect.x + X;
    aNewRect.y = aOldRect.y + Y;
    aNewRect.width = aOldRect.width;
    aNewRect.height = aOldRect.height;

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
    NS_ENSURE_TRUE(mDrawingarea, NS_OK);

    PRInt32 X,Y;

    QPoint offset(0,0);
    offset = mDrawingarea->mapFromGlobal(offset);
    X = offset.x();
    Y = offset.y();
    LOG(("WidgetToScreen (container) %d %d\n", X, Y));

    aNewRect.x = aOldRect.x - X;
    aNewRect.y = aOldRect.y - Y;
    aNewRect.width = aOldRect.width;
    aNewRect.height = aOldRect.height;

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::BeginResizingChildren(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::EndResizingChildren(void)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::EnableDragDrop(PRBool aEnable)
{
    return NS_OK;
}

void
nsWindow::ConvertToDeviceCoordinates(nscoord &aX,
                                     nscoord &aY)
{
}

NS_IMETHODIMP
nsWindow::PreCreateWidget(nsWidgetInitData *aWidgetInitData)
{
    if (nsnull != aWidgetInitData) {
        mWindowType = aWidgetInitData->mWindowType;
        mBorderStyle = aWidgetInitData->mBorderStyle;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsWindow::CaptureMouse(PRBool aCapture)
{
    LOG(("CaptureMouse %p\n", (void *)this));

    if (!mDrawingarea)
        return NS_OK;










    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureRollupEvents(nsIRollupListener *aListener,
                              PRBool             aDoCapture,
                              PRBool             aConsumeRollupEvent)
{
    if (!mDrawingarea)
        return NS_OK;

    LOG(("CaptureRollupEvents %p\n", (void *)this));










    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetAttention(PRInt32 aCycleCount)
{
    LOG(("nsWindow::GetAttention [%p]\n", (void *)this));

    SetUrgencyHint(mDrawingarea, PR_TRUE);

    return NS_OK;
}

void
nsWindow::LoseFocus(void)
{
    
    
    memset(mKeyDownFlags, 0, sizeof(mKeyDownFlags));

    
    DispatchLostFocusEvent();

    LOGFOCUS(("  widget lost focus [%p]\n", (void *)this));
}

static int gDoubleBuffering = -1;

bool
nsWindow::OnExposeEvent(QPaintEvent *aEvent)
{
    if (gDoubleBuffering == -1) {
        if (getenv("MOZ_NO_DOUBLEBUFFER"))
            gDoubleBuffering = 0;
        else
            gDoubleBuffering = 1;
    }

    if (mIsDestroyed) {
        LOG(("Expose event on destroyed window [%p] window %p\n",
             (void *)this, mDrawingarea));
        return FALSE;
    }

    if (!mDrawingarea)
        return FALSE;

    static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

    nsCOMPtr<nsIRegion> updateRegion = do_CreateInstance(kRegionCID);
    if (!updateRegion)
        return FALSE;

    updateRegion->Init();

    QVector<QRect>  rects = aEvent->region().rects();

    LOGDRAW(("sending expose event [%p] %p 0x%lx (rects follow):\n",
             (void *)this, (void *)aEvent, 0));

    for (int i = 0; i < rects.size(); ++i) {
       QRect r = rects.at(i);
       updateRegion->Union(r.x(), r.y(), r.width(), r.height());
       LOGDRAW(("\t%d %d %d %d\n", r.x(), r.y(), r.width(), r.height()));
    }

    QPainter painter(mDrawingarea);

    nsRefPtr<gfxQPainterSurface> targetSurface = new gfxQPainterSurface(&painter);
    nsRefPtr<gfxContext> ctx = new gfxContext(targetSurface);

    nsCOMPtr<nsIRenderingContext> rc;
    GetDeviceContext()->CreateRenderingContextInstance(*getter_AddRefs(rc));
    if (NS_UNLIKELY(!rc))
        return FALSE;

    rc->Init(GetDeviceContext(), ctx);

    PRBool translucent;
    GetHasTransparentBackground(translucent);
    nsIntRect boundsRect;

    updateRegion->GetBoundingBox(&boundsRect.x, &boundsRect.y,
                                 &boundsRect.width, &boundsRect.height);

    
    ctx->Save();
    ctx->NewPath();
    if (translucent) {
        
        
        
        
        ctx->Rectangle(gfxRect(boundsRect.x, boundsRect.y,
                               boundsRect.width, boundsRect.height));
    } else {
        for (int i = 0; i < rects.size(); ++i) {
           QRect r = rects.at(i);
           ctx->Rectangle(gfxRect(r.x(), r.y(), r.width(), r.height()));
        }
    }
    ctx->Clip();

    
    if (translucent) {
        ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
    } else if (gDoubleBuffering) {
        ctx->PushGroup(gfxASurface::CONTENT_COLOR);
    }

#if 0
    
    
    
#ifdef DEBUG
    if (WANT_PAINT_FLASHING && aEvent->window)
        gdk_window_flash(aEvent->window, 1, 100, aEvent->region);
#endif
#endif

    nsPaintEvent event(PR_TRUE, NS_PAINT, this);
    QRect r = aEvent->rect();
    if (!r.isValid())
        r = mDrawingarea->rect();
    nsRect rect(r.x(), r.y(), r.width(), r.height());
    event.refPoint.x = aEvent->rect().x();
    event.refPoint.y = aEvent->rect().y();
    event.rect = &rect; 
    event.region = updateRegion;
    event.renderingContext = rc;

    nsEventStatus status;
    DispatchEvent(&event, status);

    
    
    if (NS_UNLIKELY(mIsDestroyed))
        return ignoreEvent(status);

    if (status == nsEventStatus_eIgnore) {
        ctx->Restore();
        return ignoreEvent(status);
    }

    if (translucent) {
        nsRefPtr<gfxPattern> pattern = ctx->PopGroup();
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
        ctx->SetPattern(pattern);
        ctx->Paint();

        nsRefPtr<gfxImageSurface> img =
            new gfxImageSurface(gfxIntSize(boundsRect.width, boundsRect.height),
                                gfxImageSurface::ImageFormatA8);
        if (img && !img->CairoStatus()) {
            img->SetDeviceOffset(gfxPoint(-boundsRect.x, -boundsRect.y));

            nsRefPtr<gfxContext> imgCtx = new gfxContext(img);
            if (imgCtx) {
                imgCtx->SetPattern(pattern);
                imgCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
                imgCtx->Paint();
            }

            UpdateTranslucentWindowAlphaInternal(nsRect(boundsRect.x, boundsRect.y,
                                                        boundsRect.width, boundsRect.height),
                                                 img->Data(), img->Stride());
        }
    } else if (gDoubleBuffering) {
        ctx->PopGroupToSource();
        ctx->Paint();
    }

    ctx->Restore();

    
    return ignoreEvent(status);
}

bool
nsWindow::OnConfigureEvent(QMoveEvent *aEvent)
{
    LOG(("configure event [%p] %d %d\n", (void *)this,
        aEvent->pos().x(),  aEvent->pos().y()));

    
    if (!mDrawingarea
        || (mBounds.x == aEvent->pos().x()
        && mBounds.y == aEvent->pos().y()))
    return FALSE;

    
    
    
    QPoint pos = aEvent->pos();
    if (mIsTopLevel) {
        mPlaced = PR_TRUE;
        
        nsRect oldrect, newrect;
        WidgetToScreen(oldrect, newrect);
        mBounds.x = newrect.x;
        mBounds.y = newrect.y;
    }

    nsGUIEvent event(PR_TRUE, NS_MOVE, this);

    event.refPoint.x = pos.x();
    event.refPoint.y = pos.y();

    
    
    nsEventStatus status;
    DispatchEvent(&event, status);

    return ignoreEvent(status);
}

bool
nsWindow::OnSizeAllocate(QResizeEvent *e)
{
    nsRect rect;

    
    GetBounds(rect);

    rect.width = e->size().width();
    rect.height = e->size().height();

    LOG(("size_allocate [%p] %d %d\n",
         (void *)this, rect.width, rect.height));

    ResizeTransparencyBitmap(rect.width, rect.height);

    mBounds.width = rect.width;
    mBounds.height = rect.height;

#ifdef DEBUG_WIDGETS
    qDebug("resizeEvent: mDrawingarea=%p, aWidth=%d, aHeight=%d, aX = %d, aY = %d", (void*)mDrawingarea,
           rect.width, rect.height, rect.x, rect.y);
#endif

    if (mTransparencyBitmap) {
      ApplyTransparencyBitmap();
    }

    if (mDrawingarea)
        mDrawingarea->resize(rect.width, rect.height);

    nsEventStatus status;
    DispatchResizeEvent(rect, status);
    return ignoreEvent(status);
}

bool
nsWindow::OnDeleteEvent(QCloseEvent *aEvent)
{
    nsGUIEvent event(PR_TRUE, NS_XUL_CLOSE, this);

    event.refPoint.x = 0;
    event.refPoint.y = 0;

    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

bool
nsWindow::OnEnterNotifyEvent(QEvent *aEvent)
{
    nsMouseEvent event(PR_TRUE, NS_MOUSE_ENTER, this, nsMouseEvent::eReal);

    QPoint pt = QCursor::pos();

    event.refPoint.x = nscoord(pt.x());
    event.refPoint.y = nscoord(pt.y());

    LOG(("OnEnterNotify: %p\n", (void *)this));

    nsEventStatus status;
    DispatchEvent(&event, status);
    return FALSE;
}

bool
nsWindow::OnLeaveNotifyEvent(QEvent *aEvent)
{
    nsMouseEvent event(PR_TRUE, NS_MOUSE_EXIT, this, nsMouseEvent::eReal);

    QPoint pt = QCursor::pos();

    event.refPoint.x = nscoord(pt.x());
    event.refPoint.y = nscoord(pt.y());

    LOG(("OnLeaveNotify: %p\n", (void *)this));

    nsEventStatus status;
    DispatchEvent(&event, status);
    return FALSE;
}

bool
nsWindow::OnMotionNotifyEvent(QMouseEvent *aEvent)
{
    
    
    

    nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, this, nsMouseEvent::eReal);


    event.refPoint.x = nscoord(aEvent->x());
    event.refPoint.y = nscoord(aEvent->y());

    event.isShift         = aEvent->modifiers() & Qt::ShiftModifier;
    event.isControl       = aEvent->modifiers() & Qt::ControlModifier;
    event.isAlt           = aEvent->modifiers() & Qt::AltModifier;
    event.isMeta          = aEvent->modifiers() & Qt::MetaModifier;
    event.clickCount      = 0;

    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

void
nsWindow::InitButtonEvent(nsMouseEvent &event,
                          QMouseEvent *aEvent, int aClickCount)
{
    event.refPoint.x = nscoord(aEvent->x());
    event.refPoint.y = nscoord(aEvent->y());

    event.isShift         = aEvent->modifiers() & Qt::ShiftModifier;
    event.isControl       = aEvent->modifiers() & Qt::ControlModifier;
    event.isAlt           = aEvent->modifiers() & Qt::AltModifier;
    event.isMeta          = aEvent->modifiers() & Qt::MetaModifier;
    event.clickCount      = aClickCount;
}

bool
nsWindow::OnButtonPressEvent(QMouseEvent *aEvent)
{
    PRUint16      domButton;
    switch (aEvent->button()) {
    case Qt::MidButton:
        domButton = nsMouseEvent::eMiddleButton;
        break;
    case Qt::RightButton:
        domButton = nsMouseEvent::eRightButton;
        break;
    default:
        domButton = nsMouseEvent::eLeftButton;
        break;
    }

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_DOWN, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent, 1);

    nsEventStatus status;
    DispatchEvent(&event, status);

    
    if (domButton == nsMouseEvent::eRightButton &&
        NS_LIKELY(!mIsDestroyed)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal);
        InitButtonEvent(contextMenuEvent, aEvent, 1);
        DispatchEvent(&contextMenuEvent, status);
    }

    return ignoreEvent(status);
}

bool
nsWindow::OnButtonReleaseEvent(QMouseEvent *aEvent)
{
    PRUint16 domButton;


    switch (aEvent->button()) {
    case Qt::MidButton:
        domButton = nsMouseEvent::eMiddleButton;
        break;
    case Qt::RightButton:
        domButton = nsMouseEvent::eRightButton;
        break;
    default:
        domButton = nsMouseEvent::eLeftButton;
        break;
    }

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_UP, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent, 1);

    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

bool
nsWindow::mouseDoubleClickEvent(QMouseEvent *e)
{
    PRUint32      eventType;

    switch (e->button()) {
    case Qt::MidButton:
        eventType = nsMouseEvent::eMiddleButton;
        break;
    case Qt::RightButton:
        eventType = nsMouseEvent::eRightButton;
        break;
    default:
        eventType = nsMouseEvent::eLeftButton;
        break;
    }

    nsMouseEvent event(PR_TRUE, NS_MOUSE_DOUBLECLICK, this, nsMouseEvent::eReal);
    event.button = eventType;

    InitButtonEvent(event, e, 2);
    
    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

bool
nsWindow::OnContainerFocusInEvent(QFocusEvent *aEvent)
{
    LOGFOCUS(("OnContainerFocusInEvent [%p]\n", (void *)this));
    
    
    

    if (!mDrawingarea)
        return FALSE;

    


    
    DispatchGotFocusEvent();

    
    
    
    DispatchActivateEvent();

    LOGFOCUS(("Events sent from focus in event [%p]\n", (void *)this));
    return FALSE;
}

bool
nsWindow::OnContainerFocusOutEvent(QFocusEvent *aEvent)
{
    LOGFOCUS(("OnContainerFocusOutEvent [%p]\n", (void *)this));

    DispatchLostFocusEvent();
    if (mDrawingarea)
        DispatchDeactivateEvent();

    LOGFOCUS(("Done with container focus out [%p]\n", (void *)this));
    return FALSE;
}

inline PRBool
is_latin_shortcut_key(quint32 aKeyval)
{
    return ((Qt::Key_0 <= aKeyval && aKeyval <= Qt::Key_9) ||
            (Qt::Key_A <= aKeyval && aKeyval <= Qt::Key_Z));
}

PRBool
nsWindow::DispatchCommandEvent(nsIAtom* aCommand)
{
    nsEventStatus status;
    nsCommandEvent event(PR_TRUE, nsWidgetAtoms::onAppCommand, aCommand, this);
    DispatchEvent(&event, status);
    return TRUE;
}

bool
nsWindow::OnKeyPressEvent(QKeyEvent *aEvent)
{
    LOGFOCUS(("OnKeyPressEvent [%p]\n", (void *)this));

    nsEventStatus status;

    nsKeyEvent event(PR_TRUE, NS_KEY_PRESS, this);
    InitKeyEvent(event, aEvent);
    event.charCode = (PRInt32)aEvent->text()[0].unicode();
    

    if (!aEvent->isAutoRepeat()) {
        
        nsKeyEvent downEvent(PR_TRUE, NS_KEY_DOWN, this);
        InitKeyEvent(downEvent, aEvent);
        DispatchEvent(&downEvent, status);
        if (ignoreEvent(status)) { 
            event.flags |= NS_EVENT_FLAG_NO_DEFAULT;
        }
    }

    
    
    if (isContextMenuKey(event)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal,
                                      nsMouseEvent::eContextMenuKey);
        keyEventToContextMenuEvent(&event, &contextMenuEvent);
        DispatchEvent(&contextMenuEvent, status);
    }
    else {
        
        DispatchEvent(&event, status);
    }


    
    LOGIM(("status %d\n", status));
    if (status == nsEventStatus_eConsumeNoDefault) {
        LOGIM(("key press consumed\n"));
        return TRUE;
    }

    return FALSE;
}

bool
nsWindow::OnKeyReleaseEvent(QKeyEvent *aEvent)
{
    LOGFOCUS(("OnKeyReleaseEvent [%p]\n", (void *)this));

    
    nsKeyEvent event(PR_TRUE, NS_KEY_UP, this);
    InitKeyEvent(event, aEvent);

    
    ClearKeyDownFlag(event.keyCode);

    nsEventStatus status;
    DispatchEvent(&event, status);

    
    if (status == nsEventStatus_eConsumeNoDefault) {
        LOGIM(("key release consumed\n"));
        return TRUE;
    }

    return FALSE;
}

bool
nsWindow::OnScrollEvent(QWheelEvent *aEvent)
{
    
    nsMouseScrollEvent event(PR_TRUE, NS_MOUSE_SCROLL, this);

    switch (aEvent->orientation()) {
    case Qt::Vertical:
        event.scrollFlags = nsMouseScrollEvent::kIsVertical;
        break;
    case Qt::Horizontal:
        event.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
        break;
    default:
        Q_ASSERT(0);
        break;
    }
    event.delta = (int)((aEvent->delta() / WHEEL_DELTA) * -3);

    event.refPoint.x = nscoord(aEvent->x());
    event.refPoint.y = nscoord(aEvent->y());

    event.isShift         = aEvent->modifiers() & Qt::ShiftModifier;
    event.isControl       = aEvent->modifiers() & Qt::ControlModifier;
    event.isAlt           = aEvent->modifiers() & Qt::AltModifier;
    event.isMeta          = aEvent->modifiers() & Qt::MetaModifier;
    event.time            = 0;

    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}


bool
nsWindow::showEvent(QShowEvent *)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__,(void *)this));
    


















    mIsVisible = PR_TRUE;
    return false;
}

bool
nsWindow::hideEvent(QHideEvent *)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__,(void *)this));
    mIsVisible = PR_FALSE;
    return false;
}

bool
nsWindow::OnWindowStateEvent(QEvent *aEvent)
{
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

void
nsWindow::ThemeChanged()
{
    nsGUIEvent event(PR_TRUE, NS_THEMECHANGED, this);
    nsEventStatus status = nsEventStatus_eIgnore;
    DispatchEvent(&event, status);

    if (!mDrawingarea || NS_UNLIKELY(mIsDestroyed))
        return;
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    return;
}

bool
nsWindow::OnDragMotionEvent(QDragMoveEvent *e)
{
    LOG(("nsWindow::OnDragMotionSignal\n"));

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, 0,
                       nsMouseEvent::eReal);
    return TRUE;
}

bool
nsWindow::OnDragLeaveEvent(QDragLeaveEvent *e)
{
    
    LOG(("nsWindow::OnDragLeaveSignal(%p)\n", this));
    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_EXIT, this, nsMouseEvent::eReal);

    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

bool
nsWindow::OnDragDropEvent(QDropEvent *e)
{
    LOG(("nsWindow::OnDragDropSignal\n"));
    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, 0,
                       nsMouseEvent::eReal);
    return TRUE;
}

bool
nsWindow::OnDragEnter(QDragEnterEvent *)
{
    

    LOG(("nsWindow::OnDragEnter(%p)\n", this));

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_ENTER, this, nsMouseEvent::eReal);
    nsEventStatus status;
    DispatchEvent(&event, status);
    return ignoreEvent(status);
}

static void
GetBrandName(nsXPIDLString& brandName)
{
    nsCOMPtr<nsIStringBundleService> bundleService = 
        do_GetService(NS_STRINGBUNDLE_CONTRACTID);

    nsCOMPtr<nsIStringBundle> bundle;
    if (bundleService)
        bundleService->CreateBundle(
            "chrome://branding/locale/brand.properties",
            getter_AddRefs(bundle));

    if (bundle)
        bundle->GetStringFromName(
            NS_LITERAL_STRING("brandShortName").get(),
            getter_Copies(brandName));

    if (brandName.IsEmpty())
        brandName.Assign(NS_LITERAL_STRING("Mozilla"));
}


nsresult
nsWindow::NativeCreate(nsIWidget        *aParent,
                       nsNativeWidget    aNativeParent,
                       const nsRect     &aRect,
                       EVENT_CALLBACK    aHandleEventFunction,
                       nsIDeviceContext *aContext,
                       nsIAppShell      *aAppShell,
                       nsIToolkit       *aToolkit,
                       nsWidgetInitData *aInitData)
{
    
    
    nsIWidget *baseParent = aInitData &&
        (aInitData->mWindowType == eWindowType_dialog ||
         aInitData->mWindowType == eWindowType_toplevel ||
         aInitData->mWindowType == eWindowType_invisible) ?
        nsnull : aParent;

    
    BaseCreate(baseParent, aRect, aHandleEventFunction, aContext,
               aAppShell, aToolkit, aInitData);

    
    PRBool listenForResizes = PR_FALSE;;
    if (aNativeParent || (aInitData && aInitData->mListenForResizes))
        listenForResizes = PR_TRUE;

    
    CommonCreate(aParent, listenForResizes);

    
    mBounds = aRect;
    if (mWindowType != eWindowType_child) {
        
        
        
        
        mNeedsMove = PR_TRUE;
    }

    
    QWidget      *parent = nsnull;
    if (aParent != nsnull)
        parent = (QWidget*)aParent->GetNativeData(NS_NATIVE_WIDGET);
    else
        parent = (QWidget*)aNativeParent;

    
    mDrawingarea = createQWidget(parent, aInitData);

    Initialize(mDrawingarea);

    LOG(("nsWindow [%p]\n", (void *)this));
    if (mDrawingarea) {
        LOG(("\tmDrawingarea %p %p %p %lx %lx\n", (void *)mDrawingarea));
    }

    
    if (!mIsTopLevel)
        Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString &xulWinType)
{
  if (!mDrawingarea)
    return NS_ERROR_FAILURE;

  nsXPIDLString brandName;
  GetBrandName(brandName);

  XClassHint *class_hint = XAllocClassHint();
  if (!class_hint)
    return NS_ERROR_OUT_OF_MEMORY;
  const char *role = NULL;
  class_hint->res_name = ToNewCString(xulWinType);
  if (!class_hint->res_name) {
    XFree(class_hint);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  class_hint->res_class = ToNewCString(brandName);
  if (!class_hint->res_class) {
    nsMemory::Free(class_hint->res_name);
    XFree(class_hint);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  
  for (char *c = class_hint->res_name; *c; c++) {
    if (':' == *c) {
      *c = 0;
      role = c + 1;
    }
    else if (!isascii(*c) || (!isalnum(*c) && ('_' != *c) && ('-' != *c)))
      *c = '_';
  }
  class_hint->res_name[0] = toupper(class_hint->res_name[0]);
  if (!role) role = class_hint->res_name;

  
  qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
  
  
  XSetClassHint(mDrawingarea->x11Info().display(),
                mDrawingarea->handle(),
                class_hint);
  nsMemory::Free(class_hint->res_class);
  nsMemory::Free(class_hint->res_name);
  XFree(class_hint);
  return NS_OK;
}

void
nsWindow::NativeResize(PRInt32 aWidth, PRInt32 aHeight, PRBool  aRepaint)
{
    LOG(("nsWindow::NativeResize [%p] %d %d\n", (void *)this,
         aWidth, aHeight));

    ResizeTransparencyBitmap(aWidth, aHeight);

    
    mNeedsResize = PR_FALSE;

    mDrawingarea->resize( aWidth, aHeight);

    if (aRepaint) {
        if (mDrawingarea->isVisible())
            mDrawingarea->repaint();
    }
}

void
nsWindow::NativeResize(PRInt32 aX, PRInt32 aY,
                       PRInt32 aWidth, PRInt32 aHeight,
                       PRBool  aRepaint)
{
    mNeedsResize = PR_FALSE;
    mNeedsMove = PR_FALSE;

    LOG(("nsWindow::NativeResize [%p] %d %d %d %d\n", (void *)this,
         aX, aY, aWidth, aHeight));

    ResizeTransparencyBitmap(aWidth, aHeight);

    QPoint pos(aX, aY);
    if (mDrawingarea)
    {
        if (mParent && mDrawingarea->windowType() == Qt::Popup) {
            nsRect oldrect, newrect;
            oldrect.x = aX;
            oldrect.y = aY;

            mParent->WidgetToScreen(oldrect, newrect);

            pos = QPoint(newrect.x, newrect.y);
#ifdef DEBUG_WIDGETS
            qDebug("pos is [%d,%d]", pos.x(), pos.y());
#endif
        } else {
#ifdef DEBUG_WIDGETS
            qDebug("Widget with original position? (%p)", mDrawingarea);
#endif
        }
    }

    mDrawingarea->setGeometry(pos.x(), pos.y(), aWidth, aHeight);

    if (aRepaint) {
        if (mDrawingarea->isVisible())
            mDrawingarea->repaint();
    }
}

void
nsWindow::NativeShow (PRBool  aAction)
{
    if (aAction) {
        
        
        
        
        
        
        
        
        if (mTransparencyBitmap) {
            ApplyTransparencyBitmap();
        }

        
        mNeedsShow = PR_FALSE;
    }
    if (!mDrawingarea) {
        
        
        qDebug("nsCommon::Show : widget empty");
        return;
    }
    mDrawingarea->setShown(aAction);
}

void
nsWindow::EnsureGrabs(void)
{
    if (mRetryPointerGrab)
        GrabPointer();
    if (mRetryKeyboardGrab)
        GrabKeyboard();
}

NS_IMETHODIMP
nsWindow::SetHasTransparentBackground(PRBool aTransparent)
{

        



    if (mIsTransparent == aTransparent)
        return NS_OK;

    if (!aTransparent) {
        if (mTransparencyBitmap) {
            delete[] mTransparencyBitmap;
            mTransparencyBitmap = nsnull;
            mTransparencyBitmapWidth = 0;
            mTransparencyBitmapHeight = 0;
            
        }
    } 
    

    mIsTransparent = aTransparent;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetHasTransparentBackground(PRBool& aTransparent)
{
    if (!mDrawingarea) {
        













    }

    aTransparent = mIsTransparent;
    return NS_OK;
}

void
nsWindow::ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight)
{
    if (!mTransparencyBitmap)
        return;

    if (aNewWidth == mTransparencyBitmapWidth &&
        aNewHeight == mTransparencyBitmapHeight)
        return;

    PRInt32 newSize = ((aNewWidth+7)/8)*aNewHeight;
    char* newBits = new char[newSize];
    if (!newBits) {
        delete[] mTransparencyBitmap;
        mTransparencyBitmap = nsnull;
        mTransparencyBitmapWidth = 0;
        mTransparencyBitmapHeight = 0;
        return;
    }
    
    memset(newBits, 255, newSize);

    
    PRInt32 copyWidth = PR_MIN(aNewWidth, mTransparencyBitmapWidth);
    PRInt32 copyHeight = PR_MIN(aNewHeight, mTransparencyBitmapHeight);
    PRInt32 oldRowBytes = (mTransparencyBitmapWidth+7)/8;
    PRInt32 newRowBytes = (aNewWidth+7)/8;
    PRInt32 copyBytes = (copyWidth+7)/8;

    PRInt32 i;
    char* fromPtr = mTransparencyBitmap;
    char* toPtr = newBits;
    for (i = 0; i < copyHeight; i++) {
        memcpy(toPtr, fromPtr, copyBytes);
        fromPtr += oldRowBytes;
        toPtr += newRowBytes;
    }

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = newBits;
    mTransparencyBitmapWidth = aNewWidth;
    mTransparencyBitmapHeight = aNewHeight;
}

static PRBool
ChangedMaskBits(char* aMaskBits, PRInt32 aMaskWidth, PRInt32 aMaskHeight,
        const nsRect& aRect, PRUint8* aAlphas, PRInt32 aStride)
{
    PRInt32 x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
    PRInt32 maskBytesPerRow = (aMaskWidth + 7)/8;
    for (y = aRect.y; y < yMax; y++) {
        char* maskBytes = aMaskBits + y*maskBytesPerRow;
        PRUint8* alphas = aAlphas;
        for (x = aRect.x; x < xMax; x++) {
            PRBool newBit = *alphas > 0;
            alphas++;

            char maskByte = maskBytes[x >> 3];
            PRBool maskBit = (maskByte & (1 << (x & 7))) != 0;

            if (maskBit != newBit) {
                return PR_TRUE;
            }
        }
        aAlphas += aStride;
    }

    return PR_FALSE;
}

static
void UpdateMaskBits(char* aMaskBits, PRInt32 aMaskWidth, PRInt32 aMaskHeight,
        const nsRect& aRect, PRUint8* aAlphas, PRInt32 aStride)
{
    PRInt32 x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
    PRInt32 maskBytesPerRow = (aMaskWidth + 7)/8;
    for (y = aRect.y; y < yMax; y++) {
        char* maskBytes = aMaskBits + y*maskBytesPerRow;
        PRUint8* alphas = aAlphas;
        for (x = aRect.x; x < xMax; x++) {
            PRBool newBit = *alphas > 0;
            alphas++;

            char mask = 1 << (x & 7);
            char maskByte = maskBytes[x >> 3];
            
            maskBytes[x >> 3] = (maskByte & ~mask) | (-newBit & mask);
        }
        aAlphas += aStride;
    }
}

void
nsWindow::ApplyTransparencyBitmap()
{
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);











}

nsresult
nsWindow::UpdateTranslucentWindowAlphaInternal(const nsRect& aRect,
                                               PRUint8* aAlphas, PRInt32 aStride)
{
    if (!mDrawingarea) {
        

        return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(mIsTransparent, "Window is not transparent");

    if (mTransparencyBitmap == nsnull) {
        PRInt32 size = ((mBounds.width+7)/8)*mBounds.height;
        mTransparencyBitmap = new char[size];
        if (mTransparencyBitmap == nsnull)
            return NS_ERROR_FAILURE;
        memset(mTransparencyBitmap, 255, size);
        mTransparencyBitmapWidth = mBounds.width;
        mTransparencyBitmapHeight = mBounds.height;
    }

    NS_ASSERTION(aRect.x >= 0 && aRect.y >= 0
            && aRect.XMost() <= mBounds.width && aRect.YMost() <= mBounds.height,
            "Rect is out of window bounds");

    if (!ChangedMaskBits(mTransparencyBitmap, mBounds.width, mBounds.height,
                         aRect, aAlphas, aStride))
        
        
        return NS_OK;

    UpdateMaskBits(mTransparencyBitmap, mBounds.width, mBounds.height,
                   aRect, aAlphas, aStride);

    if (!mNeedsShow) {
        ApplyTransparencyBitmap();
    }
    return NS_OK;
}

void
nsWindow::GrabPointer(void)
{
    LOG(("GrabPointer %d\n", mRetryPointerGrab));

    mRetryPointerGrab = PR_FALSE;

    
    
    
    PRBool visibility = PR_TRUE;
    IsVisible(visibility);
    if (!visibility) {
        LOG(("GrabPointer: window not visible\n"));
        mRetryPointerGrab = PR_TRUE;
        return;
    }

    if (!mDrawingarea)
        return;

    mDrawingarea->grabMouse();
}

void
nsWindow::GrabKeyboard(void)
{
    LOG(("GrabKeyboard %d\n", mRetryKeyboardGrab));

    mRetryKeyboardGrab = PR_FALSE;

    
    
    
    PRBool visibility = PR_TRUE;
    IsVisible(visibility);
    if (!visibility) {
        LOG(("GrabKeyboard: window not visible\n"));
        mRetryKeyboardGrab = PR_TRUE;
        return;
    }

    if (!mDrawingarea)
        return;

    mDrawingarea->grabKeyboard();
}

void
nsWindow::ReleaseGrabs(void)
{
    LOG(("ReleaseGrabs\n"));

    mRetryPointerGrab = PR_FALSE;
    mRetryKeyboardGrab = PR_FALSE;



}

void
nsWindow::GetToplevelWidget(QWidget **aWidget)
{
    *aWidget = nsnull;

    if (mDrawingarea) {
        *aWidget = mDrawingarea;
        return;
    }
}

void
nsWindow::SetUrgencyHint(QWidget *top_window, PRBool state)
{
    if (!top_window)
        return;
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);















}

void *
nsWindow::SetupPluginPort(void)
{
    if (!mDrawingarea)
        return nsnull;

    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);






















    return nsnull;
}

nsresult
nsWindow::SetWindowIconList(const nsCStringArray &aIconList)
{
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    return NS_OK;
}

void
nsWindow::SetDefaultIcon(void)
{
    SetIcon(NS_LITERAL_STRING("default"));
}

void
nsWindow::SetPluginType(PluginType aPluginType)
{
    mPluginType = aPluginType;
}

void
nsWindow::SetNonXEmbedPluginFocus()
{
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

void
nsWindow::LoseNonXEmbedPluginFocus()
{
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    LOGFOCUS(("nsWindow::LoseNonXEmbedPluginFocus\n"));
    LOGFOCUS(("nsWindow::LoseNonXEmbedPluginFocus end\n"));
}


qint32
nsWindow::ConvertBorderStyles(nsBorderStyle aStyle)
{
    qint32 w = 0;

    if (aStyle == eBorderStyle_default)
        return -1;

    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);





















    return w;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(PRBool aFullScreen)
{









    return nsBaseWidget::MakeFullScreen(aFullScreen);

}

NS_IMETHODIMP
nsWindow::HideWindowChrome(PRBool aShouldHide)
{
    if (!mDrawingarea) {
        
        QWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);

        return NS_ERROR_FAILURE;
    }

    
    
    
    PRBool wasVisible = PR_FALSE;
    if (mDrawingarea->isVisible()) {
        mDrawingarea->hide();
        wasVisible = PR_TRUE;
    }

    qint32 wmd;
    if (aShouldHide)
        wmd = 0;
    else
        wmd = ConvertBorderStyles(mBorderStyle);



    if (wasVisible) {
        mDrawingarea->show();
    }

    
    
    
    
    
    XSync(mDrawingarea->x11Info().display(), False);

    return NS_OK;
}























































void
nsWindow::InitDragEvent(nsMouseEvent &aEvent)
{
    










}






nsresult
initialize_prefs(void)
{
    
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return NS_OK;

    PRBool val = PR_TRUE;
    nsresult rv;
    rv = prefs->GetBoolPref("mozilla.widget.raise-on-setfocus", &val);

    return NS_OK;
}

inline PRBool
is_context_menu_key(const nsKeyEvent& aKeyEvent)
{
    return ((aKeyEvent.keyCode == NS_VK_F10 && aKeyEvent.isShift &&
             !aKeyEvent.isControl && !aKeyEvent.isMeta && !aKeyEvent.isAlt) ||
            (aKeyEvent.keyCode == NS_VK_CONTEXT_MENU && !aKeyEvent.isShift &&
             !aKeyEvent.isControl && !aKeyEvent.isMeta && !aKeyEvent.isAlt));
}

void
key_event_to_context_menu_event(nsMouseEvent &aEvent,
                                QKeyEvent *aGdkEvent)
{
    aEvent.refPoint = nsPoint(0, 0);
    aEvent.isShift = PR_FALSE;
    aEvent.isControl = PR_FALSE;
    aEvent.isAlt = PR_FALSE;
    aEvent.isMeta = PR_FALSE;
    aEvent.time = 0;
    aEvent.clickCount = 1;
}



























































nsChildWindow::nsChildWindow()
{
}

nsChildWindow::~nsChildWindow()
{
}

nsPopupWindow::nsPopupWindow()
{
    qDebug("===================== popup!");
}

nsPopupWindow::~nsPopupWindow()
{
}

QWidget*
nsWindow::createQWidget(QWidget *parent, nsWidgetInitData *aInitData)
{
    Qt::WFlags flags = Qt::Widget;
#ifdef DEBUG_WIDGETS
    qDebug("NEW WIDGET\n\tparent is %p (%s)", (void*)parent,
           parent ? qPrintable(parent->objectName()) : "null");
#endif
    
    switch (mWindowType) {
    case eWindowType_dialog:
    case eWindowType_popup:
    case eWindowType_toplevel:
    case eWindowType_invisible: {
        mIsTopLevel = PR_TRUE;

        nsXPIDLString brandName;
        GetBrandName(brandName);
        NS_ConvertUTF16toUTF8 cBrand(brandName);
        if (mWindowType == eWindowType_dialog) {
            
            
            
            flags |= Qt::Dialog;
            mDrawingarea = new MozQWidget(this, parent, "topLevelDialog", flags);
            qDebug("\t\t#### dialog (%p)", (void*)mDrawingarea);
            
        }
        else if (mWindowType == eWindowType_popup) {
            flags |= Qt::Popup;
            
            
            mDrawingarea = new MozQWidget(this, parent, "topLevelPopup", flags);
            qDebug("\t\t#### popup (%p)", (void*)mDrawingarea);
            mDrawingarea->setFocusPolicy(Qt::WheelFocus);
        }
        else { 
            flags |= Qt::Window;
            mDrawingarea = new MozQWidget(this, parent, "topLevelWindow", flags);
            qDebug("\t\t#### toplevel (%p)", (void*)mDrawingarea);
            
        }
        if (mWindowType == eWindowType_popup) {
            
            
            mCursor = eCursor_wait; 
                                    
                                    
                                    
            SetCursor(eCursor_standard);
        }
    }
        break;
    case eWindowType_child: {
        mDrawingarea = new MozQWidget(this, parent, "paintArea", 0);
        qDebug("\t\t#### child (%p)", (void*)mDrawingarea);
    }
        break;
    default:
        break;
    }

    mDrawingarea->setAttribute(Qt::WA_StaticContents);
    mDrawingarea->setAttribute(Qt::WA_OpaquePaintEvent); 

    
    
    mDrawingarea->setAttribute(Qt::WA_NoSystemBackground);


    return mDrawingarea;
}


gfxASurface*
nsWindow::GetThebesSurface()
{
    
    
    
    
    mThebesSurface = nsnull;

    if (!mThebesSurface) {
#if 0
        qint32 x_offset = 0, y_offset = 0;
        qint32 width = mDrawingarea->width(), height = mDrawingarea->height();

        
        width = PR_MIN(32767, width);
        height = PR_MIN(32767, height);

        mThebesSurface = new gfxXlibSurface
            (mDrawingarea->x11Info().display(),
             (Drawable)mDrawingarea->handle(),
             static_cast<Visual*>(mDrawingarea->x11Info().visual()),
             gfxIntSize(width, height));
        
        
        if (mThebesSurface && mThebesSurface->CairoStatus() != 0)
            mThebesSurface = nsnull;

        if (mThebesSurface) {
            mThebesSurface->SetDeviceOffset(gfxPoint(-x_offset, -y_offset));
        }
#else
        mThebesSurface = new gfxQPainterSurface(gfxIntSize(5,5));
#endif
    }

    return mThebesSurface;
}

NS_IMETHODIMP
nsWindow::BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical)
{
    NS_ENSURE_ARG_POINTER(aEvent);


    if (aEvent->eventStructType != NS_MOUSE_EVENT) {
      
      return NS_ERROR_INVALID_ARG;
    }

    nsMouseEvent* mouse_event = static_cast<nsMouseEvent*>(aEvent);

    if (mouse_event->button != nsMouseEvent::eLeftButton) {
      
      return NS_ERROR_INVALID_ARG;
    }

    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);

    return NS_OK;
}

bool
nsWindow::contextMenuEvent(QContextMenuEvent *)
{
    
    return false;
}

bool
nsWindow::imStartEvent(QEvent *)
{
    qWarning("XXX imStartEvent");
    return false;
}

bool
nsWindow::imComposeEvent(QEvent *)
{
    qWarning("XXX imComposeEvent");
    return false;
}

bool
nsWindow::imEndEvent(QEvent * )
{
    qWarning("XXX imComposeEvent");
    return false;
}
