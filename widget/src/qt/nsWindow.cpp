









































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

#ifdef Q_WS_X11
#include <X11/XF86keysym.h>
#endif

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

#include "gfxQtPlatform.h"
#include "gfxXlibSurface.h"
#include "gfxQPainterSurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qwidget.h>
#include <qcursor.h>
#include <qobject.h>
#include <execinfo.h>
#include <stdlib.h>

#ifdef Q_WS_X11
#include "qx11info_x11.h"
#endif

#include <execinfo.h>

#include "mozqwidget.h"


static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);


static nsresult    initialize_prefs        (void);

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);

#define NS_WINDOW_TITLE_MAX_LENGTH 4095

#define kWindowPositionSlop 20


static const int WHEEL_DELTA = 120;
static PRBool gGlobalsInitialized = PR_FALSE;

static nsCOMPtr<nsIRollupListener> gRollupListener;
static nsWeakPtr                   gRollupWindow;
static PRBool                      gConsumeRollupEvent;



static PRBool     check_for_rollup(double aMouseX, double aMouseY,
                                   PRBool aIsWheel);
static PRBool
is_mouse_in_window (QWidget* aWindow, double aMouseX, double aMouseY);

static PRBool
isContextMenuKeyEvent(const QKeyEvent *qe)
{
    PRUint32 kc = QtKeyCodeToDOMKeyCode(qe->key());
    if (qe->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return PR_FALSE;

    PRBool isShift = qe->modifiers() & Qt::ShiftModifier;
    return (kc == NS_VK_F10 && isShift) ||
        (kc == NS_VK_CONTEXT_MENU && !isShift);
}

static void
InitKeyEvent(nsKeyEvent &aEvent, QKeyEvent *aQEvent)
{
    aEvent.isShift   = aQEvent->modifiers() & Qt::ShiftModifier;
    aEvent.isControl = aQEvent->modifiers() & Qt::ControlModifier;
    aEvent.isAlt     = aQEvent->modifiers() & Qt::AltModifier;
    aEvent.isMeta    = aQEvent->modifiers() & Qt::MetaModifier;
    aEvent.time      = 0;

    
    
    
    
    aEvent.nativeMsg = (void *)aQEvent;
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
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));
    
    mIsTopLevel       = PR_FALSE;
    mIsDestroyed      = PR_FALSE;
    mIsShown          = PR_FALSE;
    mEnabled          = PR_TRUE;

    mDrawingArea         = nsnull;
    mIsVisible           = PR_FALSE;
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

    mCursor = eCursor_standard;
}

nsWindow::~nsWindow()
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));
    
    Destroy();
}





void
nsWindow::Initialize(QWidget *widget)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));

    Q_ASSERT(widget);

    mDrawingArea = widget;
    mDrawingArea->setMouseTracking(PR_TRUE);
    mDrawingArea->setFocusPolicy(Qt::WheelFocus);
}

 void
nsWindow::ReleaseGlobals()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsBaseWidget, nsISupportsWeakReference)

NS_IMETHODIMP
nsWindow::Create(nsIWidget        *aParent,
                 const nsIntRect     &aRect,
                 EVENT_CALLBACK   aHandleEventFunction,
                 nsIDeviceContext *aContext,
                 nsIAppShell      *aAppShell,
                 nsIToolkit       *aToolkit,
                 nsWidgetInitData *aInitData)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));

    nsresult rv = NativeCreate(aParent, nsnull, aRect, aHandleEventFunction,
                               aContext, aAppShell, aToolkit, aInitData);
    return rv;
}

NS_IMETHODIMP
nsWindow::Create(nsNativeWidget aParent,
                 const nsIntRect     &aRect,
                 EVENT_CALLBACK   aHandleEventFunction,
                 nsIDeviceContext *aContext,
                 nsIAppShell      *aAppShell,
                 nsIToolkit       *aToolkit,
                 nsWidgetInitData *aInitData)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));

    nsresult rv = NativeCreate(nsnull, aParent, aRect, aHandleEventFunction,
                               aContext, aAppShell, aToolkit, aInitData);
    return rv;
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    if (mIsDestroyed || !mDrawingArea)
        return NS_OK;

    LOG(("nsWindow::Destroy [%p]\n", (void *)this));
    mIsDestroyed = PR_TRUE;

    nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);
    if (static_cast<nsIWidget *>(this) == rollupWidget.get()) {
        if (gRollupListener)
            gRollupListener->Rollup(nsnull);
        gRollupWindow = nsnull;
        gRollupListener = nsnull;
    }

    Show(PR_FALSE);

    
    
    
    for (nsIWidget* kid = mFirstChild; kid; ) {
        nsIWidget* next = kid->GetNextSibling();
        kid->Destroy();
        kid = next;
    }

    
    
    mThebesSurface = nsnull;

    if (mMozQWidget) {
        mMozQWidget->dropReceiver();

        
        
        
        mMozQWidget->deleteLater();
    }

    mDrawingArea = nsnull;

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

    if (mDrawingArea) {
        qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
        
    } else {
        NS_NOTREACHED("nsWindow::SetParent - reparenting a non-child window");
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetModal(PRBool aModal)
{
    LOG(("nsWindow::SetModal [%p] %d, widget[%p]\n", (void *)this, aModal, mDrawingArea));

    MozQWidget *mozWidget = static_cast<MozQWidget*>(mDrawingArea);
    if (mozWidget)
        mozWidget->setModal(aModal);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsVisible(PRBool & aState)
{
    aState = mDrawingArea?mDrawingArea->isVisible():PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY)
{
    if (mDrawingArea) {
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

    
    
    
    
    if (aX == mBounds.x && aY == mBounds.y &&
        mWindowType != eWindowType_popup)
        return NS_OK;

    


    if (!mDrawingArea)
        return NS_OK;

    QPoint pos(aX, aY);
    if (mDrawingArea) {
        if (mParent && mDrawingArea->windowType() == Qt::Popup) {
            nsIntPoint screenPos = mParent->WidgetToScreenOffset();
            pos += QPoint(screenPos.x, screenPos.y);
#ifdef DEBUG_WIDGETS
            qDebug("pos is [%d,%d]", pos.x(), pos.y());
#endif
        } else {
            qDebug("Widget within another? (%p)", (void*)mDrawingArea);
        }
    }

    mBounds.x = pos.x();
    mBounds.y = pos.y();

    mDrawingArea->move(pos);

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

    NS_ASSERTION(!mDrawingArea, "Expected Mozilla child widget");

    
    

    if (!GetNextSibling()) {
        
        if (mDrawingArea) {
            qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
            
        }
    } else {
        
        for (nsWindow* w = this; w;
             w = static_cast<nsWindow*>(w->GetPrevSibling())) {
            if (w->mDrawingArea) {
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

    
    
    if (!mDrawingArea || mSizeState == mSizeMode) {
        return rv;
    }

    switch (aMode) {
    case nsSizeMode_Maximized:
        mDrawingArea->showMaximized();
        break;
    case nsSizeMode_Minimized:
        mDrawingArea->showMinimized();
        break;
    default:
        
        mDrawingArea->showNormal ();
        
        
        
        
        
        break;
    }

    mSizeState = mSizeMode;

    return rv;
}

typedef void (* SetUserTimeFunc)(QWidget* aWindow, quint32 aTimestamp);


































NS_IMETHODIMP
nsWindow::SetFocus(PRBool aRaise)
{
    
    

    LOGFOCUS(("  SetFocus [%p]\n", (void *)this));

    if (!mDrawingArea)
        return NS_ERROR_FAILURE;

    if (aRaise)
        mDrawingArea->raise();
    mDrawingArea->setFocus();

    
    

    LOGFOCUS(("  widget now has focus - dispatching events [%p]\n",
              (void *)this));

    DispatchGotFocusEvent();

    LOGFOCUS(("  done dispatching events in SetFocus() [%p]\n",
              (void *)this));

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetScreenBounds(nsIntRect &aRect)
{
    aRect = nsIntRect(WidgetToScreenOffset(), mBounds.Size());
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
    if (mMozQWidget)
        mMozQWidget->SetCursor(mCursor);
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
    
    
    if (!mDrawingArea)
        return NS_OK;

    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(PRBool aIsSynchronous)
{
    LOGDRAW(("Invalidate (all) [%p]: \n", (void *)this));

    if (!mDrawingArea)
        return NS_OK;

    if (aIsSynchronous && !mDrawingArea->paintingActive())
        mDrawingArea->repaint();
    else
        mDrawingArea->update();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsIntRect &aRect,
                     PRBool        aIsSynchronous)
{
    LOGDRAW(("Invalidate (rect) [%p,%p]: %d %d %d %d (sync: %d)\n", (void *)this,
             (void*)mDrawingArea,aRect.x, aRect.y, aRect.width, aRect.height, aIsSynchronous));

    if (!mDrawingArea)
        return NS_OK;

    if (aIsSynchronous)
        mDrawingArea->repaint(aRect.x, aRect.y, aRect.width, aRect.height);
    else {
        mDrawingArea->update(aRect.x, aRect.y, aRect.width, aRect.height);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Update()
{
    if (!mDrawingArea)
        return NS_OK;

    
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Scroll(PRInt32  aDx,
                 PRInt32  aDy,
                 nsIntRect  *aClipRect)
{
    if (!mDrawingArea)
        return NS_OK;

    mDrawingArea->scroll(aDx, aDy);

    
    for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
        nsIntRect bounds;
        kid->GetBounds(bounds);
        bounds.x += aDx;
        bounds.y += aDy;
        static_cast<nsBaseWidget*>(kid)->SetBounds(bounds);
    }

    return NS_OK;
}

void*
nsWindow::GetNativeData(PRUint32 aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_WIDGET: {
        if (!mDrawingArea)
            return nsnull;

        return mDrawingArea;
        break;
    }

    case NS_NATIVE_PLUGIN_PORT:
        return SetupPluginPort();
        break;

#ifdef Q_WS_X11
    case NS_NATIVE_DISPLAY:
        return mDrawingArea->x11Info().display();
        break;
#endif

    case NS_NATIVE_GRAPHIC: {
        NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
        return (void *)static_cast<nsToolkit *>(mToolkit)->GetSharedGC();
        break;
    }

    case NS_NATIVE_SHELLWIDGET:
        return (void *) mDrawingArea;

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
    if (mDrawingArea) {
        QString qStr(QString::fromUtf16(aTitle.BeginReading(), aTitle.Length()));
        mDrawingArea->setWindowTitle(qStr);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIcon(const nsAString& aIconSpec)
{
    if (!mDrawingArea)
        return NS_OK;

    nsCOMPtr<nsILocalFile> iconFile;
    nsCAutoString path;
    nsTArray<nsCString> iconList;

    
    
    

    const char extensions[6][7] = { ".png", "16.png", "32.png", "48.png",
                                    ".xpm", "16.xpm" };

    for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(extensions); i++) {
        
        if (i == NS_ARRAY_LENGTH(extensions) - 2 && iconList.Length())
            break;

        nsAutoString extension;
        extension.AppendASCII(extensions[i]);

        ResolveIconName(aIconSpec, extension, getter_AddRefs(iconFile));
        if (iconFile) {
            iconFile->GetNativePath(path);
            iconList.AppendElement(path);
        }
    }

    
    if (iconList.Length() == 0)
        return NS_OK;

    return SetWindowIconList(iconList);
}

nsIntPoint
nsWindow::WidgetToScreenOffset()
{
    NS_ENSURE_TRUE(mDrawingArea, nsIntPoint(0,0));

    QPoint origin(0, 0);
    origin = mDrawingArea->mapToGlobal(origin);

    return nsIntPoint(origin.x(), origin.y());
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
    mDrawingArea->setAcceptDrops(aEnable);
    return NS_OK;
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

    if (!mDrawingArea)
        return NS_OK;

    if (aCapture)
        mDrawingArea->grabMouse();
    else
        mDrawingArea->releaseMouse();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureRollupEvents(nsIRollupListener *aListener,
                              PRBool             aDoCapture,
                              PRBool             aConsumeRollupEvent)
{
    if (!mDrawingArea)
        return NS_OK;

    LOG(("CaptureRollupEvents %p\n", (void *)this));

    if (aDoCapture) {
        gConsumeRollupEvent = aConsumeRollupEvent;
        gRollupListener = aListener;
        gRollupWindow = do_GetWeakReference(static_cast<nsIWidget*>(this));
    }
    else {
        gRollupListener = nsnull;
        gRollupWindow = nsnull;
    }

    return NS_OK;
}

PRBool
check_for_rollup(double aMouseX, double aMouseY,
                 PRBool aIsWheel)
{
    PRBool retVal = PR_FALSE;
    nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);

    if (rollupWidget && gRollupListener) {
        QWidget *currentPopup =
            (QWidget *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);

        if (!is_mouse_in_window(currentPopup, aMouseX, aMouseY)) {
            PRBool rollup = PR_TRUE;
            if (aIsWheel) {
                gRollupListener->ShouldRollupOnMouseWheelEvent(&rollup);
                retVal = PR_TRUE;
            }
            
            
            
            nsCOMPtr<nsIMenuRollup> menuRollup;
            menuRollup = (do_QueryInterface(gRollupListener));
            if (menuRollup) {
                nsAutoTArray<nsIWidget*, 5> widgetChain;
                menuRollup->GetSubmenuWidgetChain(&widgetChain);
                for (PRUint32 i=0; i<widgetChain.Length(); ++i) {
                    nsIWidget* widget =  widgetChain[i];
                    QWidget* currWindow =
                        (QWidget*) widget->GetNativeData(NS_NATIVE_WINDOW);
                    if (is_mouse_in_window(currWindow, aMouseX, aMouseY)) {
                       rollup = PR_FALSE;
                       break;
                    }
                } 
            } 

            
            if (rollup) {
                gRollupListener->Rollup(nsnull);
                retVal = PR_TRUE;
            }
        }
    } else {
        gRollupWindow = nsnull;
        gRollupListener = nsnull;
    }

    return retVal;
}


PRBool
is_mouse_in_window (QWidget* aWindow, double aMouseX, double aMouseY)
{
    int x = 0;
    int y = 0;
    int w, h;

    x = aWindow->pos().x();
    y = aWindow->pos().y();
    w = aWindow->size().width();
    h = aWindow->size().height();

    if (aMouseX > x && aMouseX < x + w &&
        aMouseY > y && aMouseY < y + h)
        return PR_TRUE;

    return PR_FALSE;
}

NS_IMETHODIMP
nsWindow::GetAttention(PRInt32 aCycleCount)
{
    LOG(("nsWindow::GetAttention [%p]\n", (void *)this));

    SetUrgencyHint(mDrawingArea, PR_TRUE);

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

nsEventStatus
nsWindow::OnPaintEvent(QPaintEvent *aEvent)
{
    

    if (mIsDestroyed) {
        LOG(("Expose event on destroyed window [%p] window %p\n",
             (void *)this, mDrawingArea));
        return nsEventStatus_eIgnore;
    }

    if (!mDrawingArea)
        return nsEventStatus_eIgnore;

    static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

    nsCOMPtr<nsIRegion> updateRegion = do_CreateInstance(kRegionCID);
    if (!updateRegion)
        return nsEventStatus_eIgnore;

    updateRegion->Init();

    QVector<QRect>  rects = aEvent->region().rects();

    LOGDRAW(("[%p] sending expose event %p 0x%lx (rects follow):\n",
             (void *)this, (void *)aEvent, 0));

    for (int i = 0; i < rects.size(); ++i) {
       QRect r = rects.at(i);
       updateRegion->Union(r.x(), r.y(), r.width(), r.height());
       LOGDRAW(("\t%d %d %d %d\n", r.x(), r.y(), r.width(), r.height()));
    }

    QPainter painter;

    if (!painter.begin(mDrawingArea)) {
        fprintf (stderr, "*********** Failed to begin painting!\n");
        return nsEventStatus_eConsumeNoDefault;
    }

    nsRefPtr<gfxQPainterSurface> targetSurface = new gfxQPainterSurface(&painter);
    nsRefPtr<gfxContext> ctx = new gfxContext(targetSurface);

    nsCOMPtr<nsIRenderingContext> rc;
    GetDeviceContext()->CreateRenderingContextInstance(*getter_AddRefs(rc));
    if (NS_UNLIKELY(!rc))
        return nsEventStatus_eIgnore;

    rc->Init(GetDeviceContext(), ctx);

    nsIntRect boundsRect;

    updateRegion->GetBoundingBox(&boundsRect.x, &boundsRect.y,
                                 &boundsRect.width, &boundsRect.height);

    nsPaintEvent event(PR_TRUE, NS_PAINT, this);
    QRect r = aEvent->rect();
    if (!r.isValid())
        r = mDrawingArea->rect();
    nsIntRect rect(r.x(), r.y(), r.width(), r.height());
    event.refPoint.x = aEvent->rect().x();
    event.refPoint.y = aEvent->rect().y();
    event.rect = &rect; 
    event.region = updateRegion;
    event.renderingContext = rc;

    nsEventStatus status = DispatchEvent(&event);
    

    
    
    if (NS_UNLIKELY(mIsDestroyed))
        return status;

    if (status == nsEventStatus_eIgnore)
        return status;

    LOGDRAW(("[%p] draw done\n", this));

    ctx = nsnull;
    targetSurface = nsnull;

    

    
    return status;
}

nsEventStatus
nsWindow::OnMoveEvent(QMoveEvent *aEvent)
{
    LOG(("configure event [%p] %d %d\n", (void *)this,
        aEvent->pos().x(),  aEvent->pos().y()));

    
    if (!mDrawingArea)
        return nsEventStatus_eIgnore;

    if ((mBounds.x == aEvent->pos().x() &&
         mBounds.y == aEvent->pos().y()))
    {
        return nsEventStatus_eIgnore;
    }

    
    
    
    QPoint pos = aEvent->pos();
    if (mIsTopLevel) {
        
        mBounds.MoveTo(WidgetToScreenOffset());
    }

    nsGUIEvent event(PR_TRUE, NS_MOVE, this);

    event.refPoint.x = pos.x();
    event.refPoint.y = pos.y();

    
    
    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnResizeEvent(QResizeEvent *e)
{
    nsIntRect rect;

    
    GetBounds(rect);

    rect.width = e->size().width();
    rect.height = e->size().height();

    LOG(("size_allocate [%p] %d %d\n",
         (void *)this, rect.width, rect.height));

    mBounds.width = rect.width;
    mBounds.height = rect.height;

#ifdef DEBUG_WIDGETS
    qDebug("resizeEvent: mDrawingArea=%p, aWidth=%d, aHeight=%d, aX = %d, aY = %d", (void*)mDrawingArea,
           rect.width, rect.height, rect.x, rect.y);
#endif

    if (mDrawingArea)
        mDrawingArea->resize(rect.width, rect.height);

    nsEventStatus status;
    DispatchResizeEvent(rect, status);
    return status;
}

nsEventStatus
nsWindow::OnCloseEvent(QCloseEvent *aEvent)
{
    nsGUIEvent event(PR_TRUE, NS_XUL_CLOSE, this);

    event.refPoint.x = 0;
    event.refPoint.y = 0;

    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnEnterNotifyEvent(QEvent *aEvent)
{
    nsMouseEvent event(PR_TRUE, NS_MOUSE_ENTER, this, nsMouseEvent::eReal);

    QPoint pt = QCursor::pos();

    event.refPoint.x = nscoord(pt.x());
    event.refPoint.y = nscoord(pt.y());

    LOG(("OnEnterNotify: %p\n", (void *)this));

    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnLeaveNotifyEvent(QEvent *aEvent)
{
    nsMouseEvent event(PR_TRUE, NS_MOUSE_EXIT, this, nsMouseEvent::eReal);

    QPoint pt = QCursor::pos();

    event.refPoint.x = nscoord(pt.x());
    event.refPoint.y = nscoord(pt.y());

    LOG(("OnLeaveNotify: %p\n", (void *)this));

    return DispatchEvent(&event);
}

nsEventStatus
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

    nsEventStatus status = DispatchEvent(&event);

    

    return status;
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

nsEventStatus
nsWindow::OnButtonPressEvent(QMouseEvent *aEvent)
{
    PRBool rolledUp = check_for_rollup(aEvent->globalX(),
                                       aEvent->globalY(), PR_FALSE);
    if (gConsumeRollupEvent && rolledUp)
        return nsEventStatus_eIgnore;

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

    LOG(("%s [%p] button: %d\n", __PRETTY_FUNCTION__, (void*)this, domButton));

    nsEventStatus status = DispatchEvent(&event);

    
    if (domButton == nsMouseEvent::eRightButton &&
        NS_LIKELY(!mIsDestroyed)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal);
        InitButtonEvent(contextMenuEvent, aEvent, 1);
        DispatchEvent(&contextMenuEvent, status);
    }

    

    return status;
}

nsEventStatus
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

    LOG(("%s [%p] button: %d\n", __PRETTY_FUNCTION__, (void*)this, domButton));

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_UP, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent, 1);

    nsEventStatus status = DispatchEvent(&event);

    

    return status;
}

nsEventStatus
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
    
    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnFocusInEvent(QFocusEvent *aEvent)
{
    LOGFOCUS(("OnFocusInEvent [%p]\n", (void *)this));
    
    
    

    if (!mDrawingArea)
        return nsEventStatus_eIgnore;

    


    
    DispatchGotFocusEvent();

    
    
    
    DispatchActivateEvent();

    LOGFOCUS(("Events sent from focus in event [%p]\n", (void *)this));
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnFocusOutEvent(QFocusEvent *aEvent)
{
    LOGFOCUS(("OnFocusOutEvent [%p]\n", (void *)this));

    DispatchLostFocusEvent();
    if (mDrawingArea)
        DispatchDeactivateEvent();

    LOGFOCUS(("Done with container focus out [%p]\n", (void *)this));
    return nsEventStatus_eIgnore;
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
    nsCommandEvent event(PR_TRUE, nsWidgetAtoms::onAppCommand, aCommand, this);

    DispatchEvent(&event);

    return TRUE;
}

nsEventStatus
nsWindow::OnKeyPressEvent(QKeyEvent *aEvent)
{
    LOGFOCUS(("OnKeyPressEvent [%p]\n", (void *)this));

    PRBool setNoDefault = PR_FALSE;

    
    
    if (isContextMenuKeyEvent(aEvent)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal,
                                      nsMouseEvent::eContextMenuKey);
        
        return DispatchEvent(&contextMenuEvent);
    }

    PRUint32 domCharCode = 0;
    PRUint32 domKeyCode = QtKeyCodeToDOMKeyCode(aEvent->key());

    if (aEvent->text().length() && aEvent->text()[0].isPrint())
        domCharCode = (PRInt32) aEvent->text()[0].unicode();

    
    if (!aEvent->isAutoRepeat() && !IsKeyDown(domKeyCode)) {
        

        SetKeyDownFlag(domKeyCode);

        nsKeyEvent downEvent(PR_TRUE, NS_KEY_DOWN, this);
        InitKeyEvent(downEvent, aEvent);

        downEvent.charCode = domCharCode;
        downEvent.keyCode = domCharCode ? 0 : domKeyCode;

        nsEventStatus status = DispatchEvent(&downEvent);

        
        if (status == nsEventStatus_eConsumeNoDefault)
            setNoDefault = PR_TRUE;
    }

    nsKeyEvent event(PR_TRUE, NS_KEY_PRESS, this);
    InitKeyEvent(event, aEvent);

    event.charCode = domCharCode;
    event.keyCode = domCharCode ? 0 : domKeyCode;

    if (setNoDefault)
        event.flags |= NS_EVENT_FLAG_NO_DEFAULT;

    
    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnKeyReleaseEvent(QKeyEvent *aEvent)
{
    LOGFOCUS(("OnKeyReleaseEvent [%p]\n", (void *)this));

    if (isContextMenuKeyEvent(aEvent)) {
        
        return nsEventStatus_eConsumeDoDefault;
    }

    PRUint32 domCharCode = 0;
    PRUint32 domKeyCode = QtKeyCodeToDOMKeyCode(aEvent->key());

    if (aEvent->text().length() && aEvent->text()[0].isPrint())
        domCharCode = (PRInt32) aEvent->text()[0].unicode();

    
    nsKeyEvent event(PR_TRUE, NS_KEY_UP, this);
    InitKeyEvent(event, aEvent);

    event.charCode = domCharCode;
    event.keyCode = domCharCode ? 0 : domKeyCode;

    
    ClearKeyDownFlag(event.keyCode);

    return DispatchEvent(&event);
}

nsEventStatus
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

    
    

    event.delta = (int)(aEvent->delta() / WHEEL_DELTA) * -3;

    event.refPoint.x = nscoord(aEvent->x());
    event.refPoint.y = nscoord(aEvent->y());

    event.isShift         = aEvent->modifiers() & Qt::ShiftModifier;
    event.isControl       = aEvent->modifiers() & Qt::ControlModifier;
    event.isAlt           = aEvent->modifiers() & Qt::AltModifier;
    event.isMeta          = aEvent->modifiers() & Qt::MetaModifier;
    event.time            = 0;

    return DispatchEvent(&event);
}


nsEventStatus
nsWindow::showEvent(QShowEvent *)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__,(void *)this));
    

















    mIsVisible = PR_TRUE;
    return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus
nsWindow::hideEvent(QHideEvent *)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__,(void *)this));
    mIsVisible = PR_FALSE;
    return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus
nsWindow::OnWindowStateEvent(QEvent *aEvent)
{
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);
    return DispatchEvent(&event);
}

void
nsWindow::ThemeChanged()
{
    nsGUIEvent event(PR_TRUE, NS_THEMECHANGED, this);

    DispatchEvent(&event);

    if (!mDrawingArea || NS_UNLIKELY(mIsDestroyed))
        return;
    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    return;
}

nsEventStatus
nsWindow::OnDragMotionEvent(QDragMoveEvent *e)
{
    LOG(("nsWindow::OnDragMotionSignal\n"));

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, 0,
                       nsMouseEvent::eReal);
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnDragLeaveEvent(QDragLeaveEvent *e)
{
    
    LOG(("nsWindow::OnDragLeaveSignal(%p)\n", this));
    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_EXIT, this, nsMouseEvent::eReal);

    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnDragDropEvent(QDropEvent *aDropEvent)
{
    if (aDropEvent->proposedAction() == Qt::CopyAction)
    {
        printf("text version of the data: %s\n", aDropEvent->mimeData()->text().toAscii().data());
        aDropEvent->acceptProposedAction();
    }

    LOG(("nsWindow::OnDragDropSignal\n"));
    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, 0,
                       nsMouseEvent::eReal);
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnDragEnter(QDragEnterEvent *aDragEvent)
{
#if 0
    
    QStringList strings = aDragEvent->mimeData()->formats();
    for (int i=0; i<strings.size(); ++i)
    {
        printf("%i: %s\n", i, strings.at(i).toLocal8Bit().constData());
    }
#endif

    
    if ( aDragEvent->mimeData()->hasFormat(kURLMime)
      || aDragEvent->mimeData()->hasFormat(kURLDataMime)
      || aDragEvent->mimeData()->hasFormat(kURLDescriptionMime)
      || aDragEvent->mimeData()->hasFormat(kHTMLMime)
      || aDragEvent->mimeData()->hasFormat(kUnicodeMime)
      || aDragEvent->mimeData()->hasFormat(kTextMime)
       )
    {
        aDragEvent->acceptProposedAction();
    }

    

    LOG(("nsWindow::OnDragEnter(%p)\n", this));

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_ENTER, this, nsMouseEvent::eReal);
    return DispatchEvent(&event);
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
                       const nsIntRect     &aRect,
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

    
    mParent = aParent;

    
    mBounds = aRect;

    
    QWidget      *parent = nsnull;
    if (aParent != nsnull)
        parent = (QWidget*)aParent->GetNativeData(NS_NATIVE_WIDGET);
    else
        parent = (QWidget*)aNativeParent;

    
    mDrawingArea = createQWidget(parent, aInitData);

    Initialize(mDrawingArea);

    
    if (aParent != nsnull)
    {
        mDrawingArea->setFocusPolicy(Qt::NoFocus);
    }

    LOG(("Create: nsWindow [%p] [%p]\n", (void *)this, (void *)mDrawingArea));

    
    Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString &xulWinType)
{
  if (!mDrawingArea)
    return NS_ERROR_FAILURE;

  nsXPIDLString brandName;
  GetBrandName(brandName);

#ifdef Q_WS_X11
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
  
  
  XSetClassHint(mDrawingArea->x11Info().display(),
                mDrawingArea->handle(),
                class_hint);
  nsMemory::Free(class_hint->res_class);
  nsMemory::Free(class_hint->res_name);
  XFree(class_hint);
#endif

  return NS_OK;
}

void
nsWindow::NativeResize(PRInt32 aWidth, PRInt32 aHeight, PRBool  aRepaint)
{
    LOG(("nsWindow::NativeResize [%p] %d %d\n", (void *)this,
         aWidth, aHeight));

    mDrawingArea->resize( aWidth, aHeight);

    if (aRepaint)
        mDrawingArea->update();
}

void
nsWindow::NativeResize(PRInt32 aX, PRInt32 aY,
                       PRInt32 aWidth, PRInt32 aHeight,
                       PRBool  aRepaint)
{
    LOG(("nsWindow::NativeResize [%p] %d %d %d %d\n", (void *)this,
         aX, aY, aWidth, aHeight));

    nsIntPoint pos(aX, aY);
    if (mDrawingArea)
    {
        if (mParent && mDrawingArea->windowType() == Qt::Popup) {
            pos += mParent->WidgetToScreenOffset();
#ifdef DEBUG_WIDGETS
            qDebug("pos is [%d,%d]", pos.x, pos.y);
#endif
        } else {
#ifdef DEBUG_WIDGETS
            qDebug("Widget with original position? (%p)", mDrawingArea);
#endif
        }
    }

    mDrawingArea->setGeometry(pos.x, pos.y, aWidth, aHeight);

    if (aRepaint)
        mDrawingArea->update();
}

NS_IMETHODIMP
nsWindow::SetHasTransparentBackground(PRBool aTransparent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::GetHasTransparentBackground(PRBool& aTransparent)
{
    aTransparent = mIsTransparent;
    return NS_OK;
}

void
nsWindow::GetToplevelWidget(QWidget **aWidget)
{
    *aWidget = nsnull;

    if (mDrawingArea) {
        *aWidget = mDrawingArea;
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
    if (!mDrawingArea)
        return nsnull;

    qDebug("FIXME:>>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);






















    return nsnull;
}

nsresult
nsWindow::SetWindowIconList(const nsTArray<nsCString> &aIconList)
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
    if (!mDrawingArea) {
        
        QWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);

        return NS_ERROR_FAILURE;
    }

    
    
    
    PRBool wasVisible = PR_FALSE;
    if (mDrawingArea->isVisible()) {
        mDrawingArea->hide();
        wasVisible = PR_TRUE;
    }

    qint32 wmd;
    if (aShouldHide)
        wmd = 0;
    else
        wmd = ConvertBorderStyles(mBorderStyle);



    if (wasVisible) {
        mDrawingArea->show();
    }

    
    
    
    
    
#ifdef Q_WS_X11
    XSync(mDrawingArea->x11Info().display(), False);
#endif

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
    aEvent.refPoint = nsIntPoint(0, 0);
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
    const char *windowName = NULL;

    if (gDoubleBuffering == -1) {
        if (getenv("MOZ_NO_DOUBLEBUFFER"))
            gDoubleBuffering = 0;
        else
            gDoubleBuffering = 1;
    }

#ifdef DEBUG_WIDGETS
    qDebug("NEW WIDGET\n\tparent is %p (%s)", (void*)parent,
           parent ? qPrintable(parent->objectName()) : "null");
#endif
    
    switch (mWindowType) {
    case eWindowType_dialog:
        flags |= Qt::Dialog;
        windowName = "topLevelDialog";
        break;
    case eWindowType_popup:
        flags |= Qt::ToolTip;
        windowName = "topLevelPopup";
        break;
    case eWindowType_toplevel:
        flags |= Qt::Window;
        windowName = "topLevelWindow";
        break;
    case eWindowType_invisible:
        flags |= Qt::Window;
        windowName = "topLevelInvisible";
        break;
    case eWindowType_child:
    default: 
        windowName = "paintArea";
        break;
    }

    mMozQWidget = new MozQWidget(this, parent, windowName, flags);
    mDrawingArea = mMozQWidget;
 
    if (mWindowType == eWindowType_popup) {
        mMozQWidget->setFocusPolicy(Qt::WheelFocus);
 
        
        
        
        SetCursor(eCursor_standard);
    } else if (mIsTopLevel) {
        SetDefaultIcon();
    }
 
    mMozQWidget->setAttribute(Qt::WA_StaticContents);
    mMozQWidget->setAttribute(Qt::WA_OpaquePaintEvent); 
    mMozQWidget->setAttribute(Qt::WA_NoSystemBackground);
  
    if (!gDoubleBuffering)
        mMozQWidget->setAttribute(Qt::WA_PaintOnScreen);

    return mDrawingArea;
}


gfxASurface*
nsWindow::GetThebesSurface()
{
    


    if (!mThebesSurface)
        mThebesSurface = new gfxQPainterSurface(gfxIntSize(5,5), gfxASurface::CONTENT_COLOR);

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

nsEventStatus
nsWindow::contextMenuEvent(QContextMenuEvent *)
{
    
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::imStartEvent(QEvent *)
{
    qWarning("XXX imStartEvent");
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::imComposeEvent(QEvent *)
{
    qWarning("XXX imComposeEvent");
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::imEndEvent(QEvent * )
{
    qWarning("XXX imComposeEvent");
    return nsEventStatus_eIgnore;
}

nsIWidget *
nsWindow::GetParent(void)
{
    return mParent;
}

void
nsWindow::DispatchGotFocusEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_GOTFOCUS, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::DispatchLostFocusEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_LOSTFOCUS, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::DispatchActivateEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_ACTIVATE, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::DispatchDeactivateEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_DEACTIVATE, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::DispatchResizeEvent(nsIntRect &aRect, nsEventStatus &aStatus)
{
    nsSizeEvent event(PR_TRUE, NS_SIZE, this);

    event.windowSize = &aRect;
    event.refPoint.x = aRect.x;
    event.refPoint.y = aRect.y;
    event.mWinWidth = aRect.width;
    event.mWinHeight = aRect.height;

    nsEventStatus status;
    DispatchEvent(&event, status); 
}

NS_IMETHODIMP
nsWindow::DispatchEvent(nsGUIEvent *aEvent,
                              nsEventStatus &aStatus)
{
#ifdef DEBUG
    debug_DumpEvent(stdout, aEvent->widget, aEvent,
                    nsCAutoString("something"), 0);
#endif

    aStatus = nsEventStatus_eIgnore;

    
    if (mEventCallback)
        aStatus = (* mEventCallback)(aEvent);

    
    if ((aStatus != nsEventStatus_eIgnore) && mEventListener)
        aStatus = mEventListener->ProcessEvent(*aEvent);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Show(PRBool aState)
{
    LOG(("nsWindow::Show [%p] state %d\n", (void *)this, aState));

    mIsShown = aState;

    if (!mDrawingArea)
        return NS_OK;

    mDrawingArea->setVisible(aState);
    if (mWindowType == eWindowType_popup && aState)
        Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
    mBounds.width = aWidth;
    mBounds.height = aHeight;

    if (!mDrawingArea)
        return NS_OK;

    mDrawingArea->resize(aWidth, aHeight);

    if (aRepaint)
        mDrawingArea->update();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight,
                 PRBool aRepaint)
{
    mBounds.x = aX;
    mBounds.y = aY;
    mBounds.width = aWidth;
    mBounds.height = aHeight;

    mPlaced = PR_TRUE;

    if (!mDrawingArea)
        return NS_OK;

    QPoint pos(aX, aY);

    
#if 0
    if (mParent && mDrawingArea->windowType() == Qt::Popup) {
        if (mParent->mDrawingArea)
            pos = mParent->mDrawingArea->mapToGlobal(pos);
#ifdef DEBUG_WIDGETS
        qDebug("pos is [%d,%d]", pos.x(), pos.y());
#endif
    } else {
#ifdef DEBUG_WIDGETS
        qDebug("Widget with original position? (%p)", mDrawingArea);
#endif
    }
#endif

    mDrawingArea->setGeometry(pos.x(), pos.y(), aWidth, aHeight);

    if (aRepaint)
        mDrawingArea->update();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Enable(PRBool aState)
{
    mEnabled = aState;

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsEnabled(PRBool *aState)
{
    *aState = mEnabled;

    return NS_OK;
}

void
nsWindow::OnDestroy(void)
{
    if (mOnDestroyCalled)
        return;

    mOnDestroyCalled = PR_TRUE;

    
    nsBaseWidget::OnDestroy();

    
    mParent = nsnull;

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;

    nsGUIEvent event(PR_TRUE, NS_DESTROY, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

PRBool
nsWindow::AreBoundsSane(void)
{
    if (mBounds.width > 0 && mBounds.height > 0)
        return PR_TRUE;

    return PR_FALSE;
}
