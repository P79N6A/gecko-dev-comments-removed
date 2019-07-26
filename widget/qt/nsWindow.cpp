






#include "mozilla/Util.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QtGui/QCursor>
#include <QIcon>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsSceneResizeEvent>
#include <QStyleOptionGraphicsItem>
#include <QPaintEngine>
#include <QMimeData>
#include "mozqglwidgetwrapper.h"

#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QVariant>
#include <algorithm>
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
#include <QPinchGesture>
#include <QGestureRecognizer>
#include "mozSwipeGesture.h"
static Qt::GestureType gSwipeGestureId = Qt::CustomGesture;



static const float GESTURES_BLOCK_MOUSE_FOR = 200;
#ifdef MOZ_ENABLE_QTMOBILITY
#include <QtSensors/QOrientationSensor>
using namespace QtMobility;
#endif 
#endif 

#ifdef MOZ_X11
#include <X11/Xlib.h>
#endif 

#include "nsXULAppAPI.h"

#include "prlink.h"

#include "nsWindow.h"
#include "mozqwidget.h"

#ifdef MOZ_ENABLE_QTMOBILITY
#include "mozqorientationsensorfilter.h"
#endif

#include "nsIdleService.h"
#include "nsRenderingContext.h"
#include "nsIRollupListener.h"
#include "nsWidgetsCID.h"
#include "nsQtKeyUtils.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/Likely.h"
#include "LayersTypes.h"
#include "nsIWidgetListener.h"

#include "nsIStringBundle.h"
#include "nsGfxCIID.h"

#include "imgIContainer.h"
#include "nsGfxCIID.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"

#include "gfxQtPlatform.h"
#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif
#include "gfxQPainterSurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

#include "nsIDOMSimpleGestureEvent.h" 
#include "nsIDOMWheelEvent.h"

#if MOZ_PLATFORM_MAEMO > 5
#include "nsIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIFocusManager.h"
#endif

#ifdef MOZ_X11
#include "keysym2ucs.h"
#if MOZ_PLATFORM_MAEMO == 6
#include <X11/Xatom.h>
static Atom sPluginIMEAtom = nullptr;
#define PLUGIN_VKB_REQUEST_PROP "_NPAPI_PLUGIN_REQUEST_VKB"
#include <QThread>
#endif
#endif 

#include "gfxUtils.h"
#include "Layers.h"
#include "GLContextProvider.h"
#include "BasicLayers.h"
#include "LayerManagerOGL.h"
#include "nsFastStartupQt.h"



#define PARENTLESS_WIDGET (void*)0x13579

#include "nsShmImage.h"
extern "C" {
#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"
}

using namespace mozilla;
using namespace mozilla::widget;
using mozilla::gl::GLContext;
using mozilla::layers::LayerManagerOGL;


static nsRefPtr<gfxASurface> gBufferSurface;
#ifdef MOZ_HAVE_SHMIMAGE

nsRefPtr<nsShmImage> gShmImage;
#endif

static int gBufferPixmapUsageCount = 0;
static gfxIntSize gBufferMaxSize(0, 0);


static nsresult    initialize_prefs        (void);

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);

#define NS_WINDOW_TITLE_MAX_LENGTH 4095

#define kWindowPositionSlop 20


static const int WHEEL_DELTA = 120;
static bool gGlobalsInitialized = false;

static bool
is_mouse_in_window (MozQWidget* aWindow, double aMouseX, double aMouseY);

static bool sAltGrModifier = false;

#ifdef MOZ_ENABLE_QTMOBILITY
static QOrientationSensor *gOrientation = nullptr;
static MozQOrientationSensorFilter gOrientationFilter;
#endif

static bool
isContextMenuKeyEvent(const QKeyEvent *qe)
{
    uint32_t kc = QtKeyCodeToDOMKeyCode(qe->key());
    if (qe->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return false;

    bool isShift = qe->modifiers() & Qt::ShiftModifier;
    return (kc == NS_VK_F10 && isShift) ||
        (kc == NS_VK_CONTEXT_MENU && !isShift);
}

static void
InitKeyEvent(nsKeyEvent &aEvent, QKeyEvent *aQEvent)
{
    aEvent.InitBasicModifiers(aQEvent->modifiers() & Qt::ControlModifier,
                              aQEvent->modifiers() & Qt::AltModifier,
                              aQEvent->modifiers() & Qt::ShiftModifier,
                              aQEvent->modifiers() & Qt::MetaModifier);

    
#ifdef MOZ_PLATFORM_MAEMO
    aEvent.location  = nsIDOMKeyEvent::DOM_KEY_LOCATION_MOBILE;
#endif
    aEvent.time      = 0;

    if (sAltGrModifier) {
        aEvent.modifiers |= (widget::MODIFIER_CONTROL | widget::MODIFIER_ALT);
    }

    
    
    
    
    aEvent.pluginEvent = (void *)aQEvent;
}

nsWindow::nsWindow()
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));

    mIsTopLevel       = false;
    mIsDestroyed      = false;
    mIsShown          = false;
    mEnabled          = true;
    mWidget              = nullptr;
    mIsVisible           = false;
    mActivatePending     = false;
    mWindowType          = eWindowType_child;
    mSizeState           = nsSizeMode_Normal;
    mLastSizeMode        = nsSizeMode_Normal;
    mPluginType          = PluginType_NONE;
    mQCursor             = Qt::ArrowCursor;
    mNeedsResize         = false;
    mNeedsMove           = false;
    mListenForResizes    = false;
    mNeedsShow           = false;
    mGesturesCancelled   = false;
    mTimerStarted        = false;
    mPinchEvent.needDispatch = false;
    mMoveEvent.needDispatch = false;
    
    if (!gGlobalsInitialized) {
        gfxPlatform::GetPlatform();
        gGlobalsInitialized = true;

#if defined(MOZ_X11) && (MOZ_PLATFORM_MAEMO == 6)
        
        if (QThread::currentThread() == qApp->thread()) {
            sPluginIMEAtom = XInternAtom(mozilla::DefaultXDisplay(), PLUGIN_VKB_REQUEST_PROP, False);
        }
#endif
        
        initialize_prefs();
    }

    memset(mKeyDownFlags, 0, sizeof(mKeyDownFlags));

    mIsTransparent = false;

    mCursor = eCursor_standard;

    gBufferPixmapUsageCount++;

#if (QT_VERSION > QT_VERSION_CHECK(4,6,0))
    if (gSwipeGestureId == Qt::CustomGesture) {
        
        MozSwipeGestureRecognizer* swipeRecognizer = new MozSwipeGestureRecognizer;
        gSwipeGestureId = QGestureRecognizer::registerRecognizer(swipeRecognizer);
    }
#endif
}

static inline gfxASurface::gfxImageFormat
_depth_to_gfximage_format(int32_t aDepth)
{
    switch (aDepth) {
    case 32:
        return gfxASurface::ImageFormatARGB32;
    case 24:
        return gfxASurface::ImageFormatRGB24;
    case 16:
        return gfxASurface::ImageFormatRGB16_565;
    default:
        return gfxASurface::ImageFormatUnknown;
    }
}

static inline QImage::Format
_gfximage_to_qformat(gfxASurface::gfxImageFormat aFormat)
{
    switch (aFormat) {
    case gfxASurface::ImageFormatARGB32:
        return QImage::Format_ARGB32_Premultiplied;
    case gfxASurface::ImageFormatRGB24:
        return QImage::Format_ARGB32;
    case gfxASurface::ImageFormatRGB16_565:
        return QImage::Format_RGB16;
    default:
        return QImage::Format_Invalid;
    }
}

static bool
UpdateOffScreenBuffers(int aDepth, QSize aSize, QWidget* aWidget = nullptr)
{
    gfxIntSize size(aSize.width(), aSize.height());
    if (gBufferSurface) {
        if (gBufferMaxSize.width < size.width ||
            gBufferMaxSize.height < size.height) {
            gBufferSurface = nullptr;
        } else
            return true;
    }

    gBufferMaxSize.width = std::max(gBufferMaxSize.width, size.width);
    gBufferMaxSize.height = std::max(gBufferMaxSize.height, size.height);

    
    gfxASurface::gfxImageFormat format =
        _depth_to_gfximage_format(aDepth);

    
    if (format == gfxASurface::ImageFormatUnknown)
        format = gfxASurface::ImageFormatRGB24;

#ifdef MOZ_HAVE_SHMIMAGE
    if (aWidget) {
        if (gfxPlatform::GetPlatform()->ScreenReferenceSurface()->GetType() ==
            gfxASurface::SurfaceTypeImage) {
            gShmImage = nsShmImage::Create(gBufferMaxSize,
                                           DefaultVisualOfScreen(gfxQtPlatform::GetXScreen(aWidget)),
                                           aDepth);
            gBufferSurface = gShmImage->AsSurface();
            return true;
        }
    }
#endif

    gBufferSurface = gfxPlatform::GetPlatform()->
        CreateOffscreenSurface(gBufferMaxSize, gfxASurface::ContentFromFormat(format));

    return true;
}

nsWindow::~nsWindow()
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__, (void *)this));

    Destroy();
}

 void
nsWindow::ReleaseGlobals()
{
}

NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsBaseWidget, nsISupportsWeakReference)

NS_IMETHODIMP
nsWindow::ConfigureChildren(const nsTArray<nsIWidget::Configuration>& aConfigurations)
{
    for (uint32_t i = 0; i < aConfigurations.Length(); ++i) {
        const Configuration& configuration = aConfigurations[i];

        nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
        NS_ASSERTION(w->GetParent() == this,
                     "Configured widget is not a child");

        if (w->mBounds.Size() != configuration.mBounds.Size()) {
            w->Resize(configuration.mBounds.x, configuration.mBounds.y,
                      configuration.mBounds.width, configuration.mBounds.height,
                      true);
        } else if (w->mBounds.TopLeft() != configuration.mBounds.TopLeft()) {
            w->Move(configuration.mBounds.x, configuration.mBounds.y);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    if (mIsDestroyed || !mWidget)
        return NS_OK;

    LOG(("nsWindow::Destroy [%p]\n", (void *)this));
    mIsDestroyed = true;

    if (gBufferPixmapUsageCount &&
        --gBufferPixmapUsageCount == 0) {

        gBufferSurface = nullptr;
#ifdef MOZ_HAVE_SHMIMAGE
        gShmImage = nullptr;
#endif
#ifdef MOZ_ENABLE_QTMOBILITY
        if (gOrientation) {
            gOrientation->removeFilter(&gOrientationFilter);
            gOrientation->stop();
            delete gOrientation;
            gOrientation = nullptr;
        }
#endif
    }

    
    if (mLayerManager) {
        nsRefPtr<GLContext> gl = nullptr;
        if (mLayerManager->GetBackendType() == mozilla::layers::LAYERS_OPENGL) {
            LayerManagerOGL *ogllm = static_cast<LayerManagerOGL*>(mLayerManager.get());
            gl = ogllm->gl();
        }

        mLayerManager->Destroy();

        if (gl) {
            gl->MarkDestroyed();
        }
    }
    mLayerManager = nullptr;

    
    
    
    
    
    DestroyCompositor();

    ClearCachedResources();

    nsIRollupListener* rollupListener = nsBaseWidget::GetActiveRollupListener();
    if (rollupListener) {
        nsCOMPtr<nsIWidget> rollupWidget = rollupListener->GetRollupWidget();
        if (static_cast<nsIWidget *>(this) == rollupWidget) {
            rollupListener->Rollup(0, nullptr);
        }
    }

    Show(false);

    
    
    
    for (nsIWidget* kid = mFirstChild; kid; ) {
        nsIWidget* next = kid->GetNextSibling();
        kid->Destroy();
        kid = next;
    }

    
    
    mThebesSurface = nullptr;

    QWidget *view = nullptr;
    QGraphicsScene *scene = nullptr;
    if (mWidget) {
        if (mIsTopLevel) {
            view = GetViewWidget();
            scene = mWidget->scene();
        }

        mWidget->dropReceiver();

        
        
        
        mWidget->deleteLater();
    }
    mWidget = nullptr;

    OnDestroy();

    
    delete scene;
    delete view;

    return NS_OK;
}

void
nsWindow::ClearCachedResources()
{
    if (mLayerManager &&
        mLayerManager->GetBackendType() == mozilla::layers::LAYERS_BASIC) {
        static_cast<mozilla::layers::BasicLayerManager*> (mLayerManager.get())->
            ClearCachedResources();
    }
    for (nsIWidget* kid = mFirstChild; kid; ) {
        nsIWidget* next = kid->GetNextSibling();
        static_cast<nsWindow*>(kid)->ClearCachedResources();
        kid = next;
    }
}

NS_IMETHODIMP
nsWindow::SetParent(nsIWidget *aNewParent)
{
    NS_ENSURE_ARG_POINTER(aNewParent);
    nsCOMPtr<nsIWidget> kungFuDeathGrip(this);
    nsIWidget* parent = GetParent();
    if (parent) {
        parent->RemoveChild(this);
    }
    ReparentNativeWidget(aNewParent);
    aNewParent->AddChild(this);
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ReparentNativeWidget(nsIWidget *aNewParent)
{
    NS_PRECONDITION(aNewParent, "");

    MozQWidget* newParent = static_cast<MozQWidget*>(aNewParent->GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(newParent, "Parent widget has a null native window handle");
    if (mWidget) {
        mWidget->setParentItem(newParent);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetModal(bool aModal)
{
    LOG(("nsWindow::SetModal [%p] %d, widget[%p]\n", (void *)this, aModal, mWidget));
    if (mWidget)
        mWidget->setModal(aModal);

    return NS_OK;
}

bool
nsWindow::IsVisible() const
{
    return mIsShown;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(bool aAllowSlop, int32_t *aX, int32_t *aY)
{
    if (mWidget) {
        int32_t screenWidth  = QApplication::desktop()->width();
        int32_t screenHeight = QApplication::desktop()->height();

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
nsWindow::Move(double aX, double aY)
{
    LOG(("nsWindow::Move [%p] %f %f\n", (void *)this,
         aX, aY));

    int32_t x = NSToIntRound(aX);
    int32_t y = NSToIntRound(aY);

    if (mIsTopLevel) {
        SetSizeMode(nsSizeMode_Normal);
    }

    if (x == mBounds.x && y == mBounds.y)
        return NS_OK;

    mNeedsMove = false;

    
    QPointF pos( x, y );
    if (mIsTopLevel) {
        QWidget *widget = GetViewWidget();
        NS_ENSURE_TRUE(widget, NS_OK);
        widget->move(x, y);
    }
    else if (mWidget) {
        
        
        pos = mWidget->mapFromScene(pos);
        pos = mWidget->mapToParent(pos);
        mWidget->setPos(pos);
    }

    mBounds.x = pos.x();
    mBounds.y = pos.y();

    NotifyRollupGeometryChange();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                      nsIWidget                  *aWidget,
                      bool                        aActivate)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetSizeMode(int32_t aMode)
{
    nsresult rv;

    LOG(("nsWindow::SetSizeMode [%p] %d\n", (void *)this, aMode));
    if (aMode != nsSizeMode_Minimized) {
        GetViewWidget()->activateWindow();
    }

    
    rv = nsBaseWidget::SetSizeMode(aMode);

    
    
    if (!mWidget || mSizeState == mSizeMode) {
        return rv;
    }

    QWidget *widget = GetViewWidget();
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

    switch (aMode) {
    case nsSizeMode_Maximized:
        widget->showMaximized();
        break;
    case nsSizeMode_Minimized:
        widget->showMinimized();
        break;
    case nsSizeMode_Fullscreen:
        widget->showFullScreen();
        break;

    default:
        
        widget->showNormal();
        break;
    }

    mSizeState = mSizeMode;

    return rv;
}




static void find_first_visible_parent(QGraphicsItem* aItem, QGraphicsItem*& aVisibleItem)
{
    NS_ENSURE_TRUE_VOID(aItem);

    aVisibleItem = nullptr;
    QGraphicsItem* parItem = nullptr;
    while (!aVisibleItem) {
        if (aItem->isVisible())
            aVisibleItem = aItem;
        else {
            parItem = aItem->parentItem();
            if (parItem)
                aItem = parItem;
            else {
                aItem->setVisible(true);
                aVisibleItem = aItem;
            }
        }
    }
}

NS_IMETHODIMP
nsWindow::SetFocus(bool aRaise)
{
    
    
    LOGFOCUS(("  SetFocus [%p]\n", (void *)this));

    if (!mWidget)
        return NS_ERROR_FAILURE;

    if (mWidget->hasFocus())
        return NS_OK;

    
    
    
    QGraphicsItem* realFocusItem = nullptr;
    find_first_visible_parent(mWidget, realFocusItem);

    if (!realFocusItem || realFocusItem->hasFocus())
        return NS_OK;

    if (aRaise) {
        
        QWidget *widget = GetViewWidget();
        if (widget)
            widget->raise();
        realFocusItem->setFocus(Qt::ActiveWindowFocusReason);
    }
    else
        realFocusItem->setFocus(Qt::OtherFocusReason);

    
    
    DispatchActivateEvent();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetScreenBounds(nsIntRect &aRect)
{
    aRect = nsIntRect(nsIntPoint(0, 0), mBounds.Size());
    if (mIsTopLevel) {
        QWidget *widget = GetViewWidget();
        NS_ENSURE_TRUE(widget, NS_OK);
        QPoint pos = widget->pos();
        aRect.MoveTo(pos.x(), pos.y());
    }
    else {
        aRect.MoveTo(WidgetToScreenOffset());
    }
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
    if (mCursor == aCursor)
        return NS_OK;

    mCursor = aCursor;
    if (mWidget)
        mWidget->SetCursor(mCursor);
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetCursor(imgIContainer* aCursor,
                    uint32_t aHotspotX, uint32_t aHotspotY)
{
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsIntRect &aRect)
{
    LOGDRAW(("Invalidate (rect) [%p,%p]: %d %d %d %d\n", (void *)this,
             (void*)mWidget,aRect.x, aRect.y, aRect.width, aRect.height));

    if (!mWidget)
        return NS_OK;

    mDirtyScrollArea = mDirtyScrollArea.united(QRect(aRect.x, aRect.y, aRect.width, aRect.height));

    mWidget->update(aRect.x, aRect.y, aRect.width, aRect.height);

    return NS_OK;
}





QWidget* nsWindow::GetViewWidget()
{
    NS_ASSERTION(mWidget, "Calling GetViewWidget without mWidget created");
    if (!mWidget || !mWidget->scene() || !mWidget->scene()->views().size())
        return nullptr;

    NS_ASSERTION(mWidget->scene()->views().size() == 1, "Not exactly one view for our scene!");
    return mWidget->scene()->views()[0];
}

void*
nsWindow::GetNativeData(uint32_t aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_WIDGET: {
        if (!mWidget)
            return nullptr;

        return mWidget;
        break;
    }

    case NS_NATIVE_PLUGIN_PORT:
        return SetupPluginPort();
        break;

    case NS_NATIVE_DISPLAY:
        {
#ifdef MOZ_X11
            return gfxQtPlatform::GetXDisplay(GetViewWidget());
#else
            return nullptr;
#endif
        }
        break;

    case NS_NATIVE_GRAPHIC: {
        return nullptr;
        break;
    }

    case NS_NATIVE_SHELLWIDGET: {
        QWidget* widget = nullptr;
        if (mWidget && mWidget->scene())
            widget = mWidget->scene()->views()[0]->viewport();
        return (void *) widget;
    }

    case NS_NATIVE_SHAREABLE_WINDOW: {
        QWidget *widget = GetViewWidget();
        return widget ? (void*)widget->winId() : nullptr;
    }

    default:
        NS_WARNING("nsWindow::GetNativeData called with bad value");
        return nullptr;
    }
}

NS_IMETHODIMP
nsWindow::SetTitle(const nsAString& aTitle)
{
    QString qStr(QString::fromUtf16(aTitle.BeginReading(), aTitle.Length()));
    if (mIsTopLevel) {
        QWidget *widget = GetViewWidget();
        if (widget)
            widget->setWindowTitle(qStr);
    }
    else if (mWidget)
        mWidget->setWindowTitle(qStr);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIcon(const nsAString& aIconSpec)
{
    if (!mWidget)
        return NS_OK;

    nsCOMPtr<nsIFile> iconFile;
    nsAutoCString path;
    nsTArray<nsCString> iconList;

    
    
    

    const char extensions[6][7] = { ".png", "16.png", "32.png", "48.png",
                                    ".xpm", "16.xpm" };

    for (uint32_t i = 0; i < ArrayLength(extensions); i++) {
        
        if (i == ArrayLength(extensions) - 2 && iconList.Length())
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
    NS_ENSURE_TRUE(mWidget, nsIntPoint(0,0));

    QPointF origin(0, 0);
    origin = mWidget->mapToScene(origin);

    return nsIntPoint(origin.x(), origin.y());
}

NS_IMETHODIMP
nsWindow::EnableDragDrop(bool aEnable)
{
    mWidget->setAcceptDrops(aEnable);
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureMouse(bool aCapture)
{
    LOG(("CaptureMouse %p\n", (void *)this));

    if (!mWidget)
        return NS_OK;

    QWidget *widget = GetViewWidget();
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

    if (aCapture)
        widget->grabMouse();
    else
        widget->releaseMouse();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureRollupEvents(nsIRollupListener *aListener,
                              bool               aDoCapture)
{
    if (!mWidget)
        return NS_OK;

    LOG(("CaptureRollupEvents %p\n", (void *)this));

    gRollupListener = aDoCapture ? aListener : nullptr;
    return NS_OK;
}

bool
nsWindow::CheckForRollup(double aMouseX, double aMouseY,
                         bool aIsWheel)
{
    nsIRollupListener* rollupListener = GetActiveRollupListener();
    nsCOMPtr<nsIWidget> rollupWidget;
    if (rollupListener) {
        rollupWidget = rollupListener->GetRollupWidget();
    }
    if (!rollupWidget) {
        nsBaseWidget::gRollupListener = nullptr;
        return false;
    }

    bool retVal = false;
    MozQWidget *currentPopup =
        (MozQWidget *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);
    if (!is_mouse_in_window(currentPopup, aMouseX, aMouseY)) {
        bool rollup = true;
        if (aIsWheel) {
            rollup = rollupListener->ShouldRollupOnMouseWheelEvent();
            retVal = true;
        }
        
        
        
        uint32_t popupsToRollup = UINT32_MAX;
        if (rollupListener) {
            nsAutoTArray<nsIWidget*, 5> widgetChain;
            uint32_t sameTypeCount = rollupListener->GetSubmenuWidgetChain(&widgetChain);
            for (uint32_t i=0; i<widgetChain.Length(); ++i) {
                nsIWidget* widget =  widgetChain[i];
                MozQWidget* currWindow =
                    (MozQWidget*) widget->GetNativeData(NS_NATIVE_WINDOW);
                if (is_mouse_in_window(currWindow, aMouseX, aMouseY)) {
                  if (i < sameTypeCount) {
                    rollup = false;
                  }
                  else {
                    popupsToRollup = sameTypeCount;
                  }
                  break;
                }
            } 
        } 

        
        if (rollup) {
            retVal = rollupListener->Rollup(popupsToRollup, nullptr);
        }
    }

    return retVal;
}


bool
is_mouse_in_window (MozQWidget* aWindow, double aMouseX, double aMouseY)
{
    return aWindow->geometry().contains( aMouseX, aMouseY );
}

NS_IMETHODIMP
nsWindow::GetAttention(int32_t aCycleCount)
{
    LOG(("nsWindow::GetAttention [%p]\n", (void *)this));
    return NS_ERROR_NOT_IMPLEMENTED;
}

#ifdef MOZ_X11
static already_AddRefed<gfxASurface>
GetSurfaceForQWidget(QWidget* aDrawable)
{
    gfxASurface* result =
        new gfxXlibSurface(gfxQtPlatform::GetXDisplay(aDrawable),
                           aDrawable->winId(),
                           DefaultVisualOfScreen(gfxQtPlatform::GetXScreen(aDrawable)),
                           gfxIntSize(aDrawable->size().width(),
                           aDrawable->size().height()));
    NS_IF_ADDREF(result);
    return result;
}
#endif

bool
nsWindow::DoPaint(QPainter* aPainter, const QStyleOptionGraphicsItem* aOption, QWidget* aWidget)
{
    if (mIsDestroyed) {
        LOG(("Expose event on destroyed window [%p] window %p\n",
             (void *)this, mWidget));
        return false;
    }

    
    {
        if (mWidgetListener)
            mWidgetListener->WillPaintWindow(this);
    }

    if (!mWidget)
        return false;

    QRectF r;
    if (aOption)
        r = aOption->exposedRect;
    else
        r = mWidget->boundingRect();

    if (r.isEmpty())
        return nsEventStatus_eIgnore;

    if (!mDirtyScrollArea.isEmpty())
        mDirtyScrollArea = QRegion();

    bool painted = false;
    nsIntRect rect(r.x(), r.y(), r.width(), r.height());

    nsFastStartup* startup = nsFastStartup::GetSingleton();
    if (startup) {
        startup->RemoveFakeLayout();
    }

    if (GetLayerManager(nullptr)->GetBackendType() == mozilla::layers::LAYERS_OPENGL) {
        aPainter->beginNativePainting();
        nsIntRegion region(rect);
        static_cast<mozilla::layers::LayerManagerOGL*>(GetLayerManager(nullptr))->
            SetClippingRegion(region);

        gfxMatrix matr;
        matr.Translate(gfxPoint(aPainter->transform().dx(), aPainter->transform().dy()));
#ifdef MOZ_ENABLE_QTMOBILITY
        
        
        matr.Rotate((M_PI/180) * gOrientationFilter.GetWindowRotationAngle());
        static_cast<mozilla::layers::LayerManagerOGL*>(GetLayerManager(nullptr))->
            SetWorldTransform(matr);
#endif 

        if (mWidgetListener)
          painted = mWidgetListener->PaintWindow(this, region, 0);
        aPainter->endNativePainting();
        if (mWidgetListener)
          mWidgetListener->DidPaintWindow();
        return painted;
    }

    gfxQtPlatform::RenderMode renderMode = gfxQtPlatform::GetPlatform()->GetRenderMode();
    int depth = aPainter->device()->depth();

    nsRefPtr<gfxASurface> targetSurface = nullptr;
    if (renderMode == gfxQtPlatform::RENDER_BUFFERED) {
        
        if (!UpdateOffScreenBuffers(depth, QSize(r.width(), r.height())))
            return false;

        targetSurface = gBufferSurface;

#ifdef CAIRO_HAS_QT_SURFACE
    } else if (renderMode == gfxQtPlatform::RENDER_QPAINTER) {
        targetSurface = new gfxQPainterSurface(aPainter);
#endif
    } else if (renderMode == gfxQtPlatform::RENDER_DIRECT) {
        if (!UpdateOffScreenBuffers(depth, aWidget->size(), aWidget)) {
            return false;
        }
        targetSurface = gBufferSurface;
    }

    if (MOZ_UNLIKELY(!targetSurface))
        return false;

    nsRefPtr<gfxContext> ctx = new gfxContext(targetSurface);

    
    if (renderMode == gfxQtPlatform::RENDER_BUFFERED) {
        ctx->Translate(gfxPoint(-r.x(), -r.y()));
    }
    else if (renderMode == gfxQtPlatform::RENDER_DIRECT) {
      gfxMatrix matr;
      matr.Translate(gfxPoint(aPainter->transform().dx(), aPainter->transform().dy()));
#ifdef MOZ_ENABLE_QTMOBILITY
         
         
         matr.Rotate((M_PI/180) * gOrientationFilter.GetWindowRotationAngle());
         NS_ASSERTION(PIXMAN_VERSION > PIXMAN_VERSION_ENCODE(0, 21, 2) ||
                      !gOrientationFilter.GetWindowRotationAngle(),
                      "Old pixman and rotate transform, it is going to be slow");
#endif 

      ctx->SetMatrix(matr);
    }

    {
        AutoLayerManagerSetup
            setupLayerManager(this, ctx, mozilla::layers::BUFFER_NONE);
        if (mWidgetListener) {
          nsIntRegion region(rect);
          painted = mWidgetListener->PaintWindow(this, region, 0);
        }
    }

    
    
    if (MOZ_UNLIKELY(mIsDestroyed))
        return painted;

    if (!painted)
        return false;

    LOGDRAW(("[%p] draw done\n", this));

    
    if (renderMode == gfxQtPlatform::RENDER_BUFFERED) {
#if defined(MOZ_X11) && defined(Q_WS_X11)
        if (gBufferSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
            
            static QPixmap gBufferPixmap;
            Drawable draw = static_cast<gfxXlibSurface*>(gBufferSurface.get())->XDrawable();
            if (gBufferPixmap.handle() != draw)
                gBufferPixmap = QPixmap::fromX11Pixmap(draw, QPixmap::ExplicitlyShared);
            XSync(static_cast<gfxXlibSurface*>(gBufferSurface.get())->XDisplay(), False);
            aPainter->drawPixmap(QPoint(rect.x, rect.y), gBufferPixmap,
                                 QRect(0, 0, rect.width, rect.height));

        } else
#endif
        if (gBufferSurface->GetType() == gfxASurface::SurfaceTypeImage) {
            
            gfxImageSurface *imgs = static_cast<gfxImageSurface*>(gBufferSurface.get());
            QImage img(imgs->Data(),
                       imgs->Width(),
                       imgs->Height(),
                       imgs->Stride(),
                       _gfximage_to_qformat(imgs->Format()));
            aPainter->drawImage(QPoint(rect.x, rect.y), img,
                                QRect(0, 0, rect.width, rect.height));
        }
    } else if (renderMode == gfxQtPlatform::RENDER_DIRECT) {
        QRect trans = aPainter->transform().mapRect(r).toRect();
#ifdef MOZ_X11
        if (gBufferSurface->GetType() == gfxASurface::SurfaceTypeXlib) {
            nsRefPtr<gfxASurface> widgetSurface = GetSurfaceForQWidget(aWidget);
            nsRefPtr<gfxContext> ctx = new gfxContext(widgetSurface);
            ctx->SetSource(gBufferSurface);
            ctx->Rectangle(gfxRect(trans.x(), trans.y(), trans.width(), trans.height()), true);
            ctx->Clip();
            ctx->Fill();
        } else
#endif
        if (gBufferSurface->GetType() == gfxASurface::SurfaceTypeImage) {
#ifdef MOZ_HAVE_SHMIMAGE
            if (gShmImage) {
                gShmImage->Put(aWidget, trans);
            } else
#endif
            {
                
                gfxImageSurface *imgs = static_cast<gfxImageSurface*>(gBufferSurface.get());
                QImage img(imgs->Data(),
                           imgs->Width(),
                           imgs->Height(),
                           imgs->Stride(),
                          _gfximage_to_qformat(imgs->Format()));
                aPainter->drawImage(trans, img, trans);
            }
        }
    }

    ctx = nullptr;
    targetSurface = nullptr;
    if (mWidgetListener)
      mWidgetListener->DidPaintWindow();

    
    return painted;
}

nsEventStatus
nsWindow::OnMoveEvent(QGraphicsSceneHoverEvent *aEvent)
{
    LOG(("configure event [%p] %d %d\n", (void *)this,
        aEvent->pos().x(),  aEvent->pos().y()));

    
    if (!mWidget || !mWidgetListener)
        return nsEventStatus_eIgnore;

    if ((mBounds.x == aEvent->pos().x() &&
         mBounds.y == aEvent->pos().y()))
    {
        return nsEventStatus_eIgnore;
    }

    bool moved = mWidgetListener->WindowMoved(this, aEvent->pos().x(), aEvent->pos().y());
    return moved ? nsEventStatus_eConsumeNoDefault : nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnResizeEvent(QGraphicsSceneResizeEvent *aEvent)
{
    nsIntRect rect;

    
    GetBounds(rect);

    rect.width = aEvent->newSize().width();
    rect.height = aEvent->newSize().height();

    mBounds.width = rect.width;
    mBounds.height = rect.height;

    nsEventStatus status;
    DispatchResizeEvent(rect, status);
    return status;
}

nsEventStatus
nsWindow::OnCloseEvent(QCloseEvent *aEvent)
{
    if (!mWidgetListener)
        return nsEventStatus_eIgnore;
    mWidgetListener->RequestWindowClose(this);
    return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus
nsWindow::OnEnterNotifyEvent(QGraphicsSceneHoverEvent *aEvent)
{
    nsMouseEvent event(true, NS_MOUSE_ENTER, this, nsMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->pos().x());
    event.refPoint.y = nscoord(aEvent->pos().y());

    LOG(("OnEnterNotify: %p\n", (void *)this));

    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnLeaveNotifyEvent(QGraphicsSceneHoverEvent *aEvent)
{
    nsMouseEvent event(true, NS_MOUSE_EXIT, this, nsMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->pos().x());
    event.refPoint.y = nscoord(aEvent->pos().y());

    LOG(("OnLeaveNotify: %p\n", (void *)this));

    return DispatchEvent(&event);
}



#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
#define CHECK_MOUSE_BLOCKED { \
if (mLastMultiTouchTime.isValid()) { \
    if (mLastMultiTouchTime.elapsed() < GESTURES_BLOCK_MOUSE_FOR) \
        return nsEventStatus_eIgnore; \
    else \
        mLastMultiTouchTime = QTime(); \
   } \
}
#else
define CHECK_MOUSE_BLOCKED {}
#endif

nsEventStatus
nsWindow::OnMotionNotifyEvent(QPointF aPos,  Qt::KeyboardModifiers aModifiers)
{
    UserActivity();

    CHECK_MOUSE_BLOCKED

    mMoveEvent.pos = aPos;
    mMoveEvent.modifiers = aModifiers;
    mMoveEvent.needDispatch = true;
    DispatchMotionToMainThread();

    return nsEventStatus_eIgnore;
}

void
nsWindow::InitButtonEvent(nsMouseEvent &aMoveEvent,
                          QGraphicsSceneMouseEvent *aEvent, int aClickCount)
{
    aMoveEvent.refPoint.x = nscoord(aEvent->pos().x());
    aMoveEvent.refPoint.y = nscoord(aEvent->pos().y());

    aMoveEvent.InitBasicModifiers(aEvent->modifiers() & Qt::ControlModifier,
                                  aEvent->modifiers() & Qt::AltModifier,
                                  aEvent->modifiers() & Qt::ShiftModifier,
                                  aEvent->modifiers() & Qt::MetaModifier);
    aMoveEvent.clickCount      = aClickCount;
}

nsEventStatus
nsWindow::OnButtonPressEvent(QGraphicsSceneMouseEvent *aEvent)
{
    
    UserActivity();

    CHECK_MOUSE_BLOCKED

    QPointF pos = aEvent->pos();

    
    
    if (mWidget)
        pos = mWidget->mapToParent(pos);

    if (CheckForRollup( pos.x(), pos.y(), false))
        return nsEventStatus_eIgnore;

    uint16_t      domButton;
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

    nsMouseEvent event(true, NS_MOUSE_BUTTON_DOWN, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent, 1);

    LOG(("%s [%p] button: %d\n", __PRETTY_FUNCTION__, (void*)this, domButton));

    nsEventStatus status = DispatchEvent(&event);

    
    if (domButton == nsMouseEvent::eRightButton &&
        MOZ_LIKELY(!mIsDestroyed)) {
        nsMouseEvent contextMenuEvent(true, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal);
        InitButtonEvent(contextMenuEvent, aEvent, 1);
        DispatchEvent(&contextMenuEvent, status);
    }

    return status;
}

nsEventStatus
nsWindow::OnButtonReleaseEvent(QGraphicsSceneMouseEvent *aEvent)
{
    UserActivity();
    CHECK_MOUSE_BLOCKED

    
    UserActivity();

    uint16_t domButton;

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

    nsMouseEvent event(true, NS_MOUSE_BUTTON_UP, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent, 1);

    nsEventStatus status = DispatchEvent(&event);

    return status;
}

nsEventStatus
nsWindow::OnMouseDoubleClickEvent(QGraphicsSceneMouseEvent *aEvent)
{
    uint32_t eventType;

    switch (aEvent->button()) {
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

    nsMouseEvent event(true, NS_MOUSE_DOUBLECLICK, this, nsMouseEvent::eReal);
    event.button = eventType;

    InitButtonEvent(event, aEvent, 2);
    
    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnFocusInEvent(QEvent *aEvent)
{
    LOGFOCUS(("OnFocusInEvent [%p]\n", (void *)this));

    if (!mWidget)
        return nsEventStatus_eIgnore;

    DispatchActivateEventOnTopLevelWindow();

    LOGFOCUS(("Events sent from focus in event [%p]\n", (void *)this));
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnFocusOutEvent(QEvent *aEvent)
{
    LOGFOCUS(("OnFocusOutEvent [%p]\n", (void *)this));

    if (!mWidget)
        return nsEventStatus_eIgnore;

#if MOZ_PLATFORM_MAEMO > 5
    if (((QFocusEvent*)aEvent)->reason() == Qt::OtherFocusReason
         && mWidget->isVKBOpen()) {
        
        
        nsCOMPtr<nsIFocusManager> fm = do_GetService("@mozilla.org/focus-manager;1");
        if (fm) {
            nsCOMPtr<nsIDOMWindow> domWindow;
            fm->GetActiveWindow(getter_AddRefs(domWindow));
            fm->ClearFocus(domWindow);
        }

        return nsEventStatus_eIgnore;
    }
#endif

    DispatchDeactivateEventOnTopLevelWindow();

    LOGFOCUS(("Done with container focus out [%p]\n", (void *)this));
    return nsEventStatus_eIgnore;
}

inline bool
is_latin_shortcut_key(quint32 aKeyval)
{
    return ((Qt::Key_0 <= aKeyval && aKeyval <= Qt::Key_9) ||
            (Qt::Key_A <= aKeyval && aKeyval <= Qt::Key_Z));
}

nsEventStatus
nsWindow::DispatchCommandEvent(nsIAtom* aCommand)
{
    nsCommandEvent event(true, nsGkAtoms::onAppCommand, aCommand, this);

    nsEventStatus status;
    DispatchEvent(&event, status);

    return status;
}

nsEventStatus
nsWindow::DispatchContentCommandEvent(int32_t aMsg)
{
    nsContentCommandEvent event(true, aMsg, this);

    nsEventStatus status;
    DispatchEvent(&event, status);

    return status;
}

nsEventStatus
nsWindow::OnKeyPressEvent(QKeyEvent *aEvent)
{
    LOGFOCUS(("OnKeyPressEvent [%p]\n", (void *)this));

    
    UserActivity();

    bool setNoDefault = false;

    if (aEvent->key() == Qt::Key_AltGr) {
        sAltGrModifier = true;
    }

#ifdef MOZ_X11
    
    
    if (isContextMenuKeyEvent(aEvent)) {
        nsMouseEvent contextMenuEvent(true, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal,
                                      nsMouseEvent::eContextMenuKey);
        
        return DispatchEvent(&contextMenuEvent);
    }

    uint32_t domCharCode = 0;
    uint32_t domKeyCode = QtKeyCodeToDOMKeyCode(aEvent->key());

    
    Display *display = mozilla::DefaultXDisplay();
    int x_min_keycode = 0, x_max_keycode = 0, xkeysyms_per_keycode;
    XDisplayKeycodes(display, &x_min_keycode, &x_max_keycode);
    XModifierKeymap *xmodmap = XGetModifierMapping(display);
    if (!xmodmap)
        return nsEventStatus_eIgnore;

    KeySym *xkeymap = XGetKeyboardMapping(display, x_min_keycode, x_max_keycode - x_min_keycode,
                                          &xkeysyms_per_keycode);
    if (!xkeymap) {
        XFreeModifiermap(xmodmap);
        return nsEventStatus_eIgnore;
    }

    
    qint32 shift_mask = 0, shift_lock_mask = 0, caps_lock_mask = 0, num_lock_mask = 0;

    for (int i = 0; i < 8 * xmodmap->max_keypermod; ++i) {
        qint32 maskbit = 1 << (i / xmodmap->max_keypermod);
        KeyCode modkeycode = xmodmap->modifiermap[i];
        if (modkeycode == NoSymbol) {
            continue;
        }

        quint32 mapindex = (modkeycode - x_min_keycode) * xkeysyms_per_keycode;
        for (int j = 0; j < xkeysyms_per_keycode; ++j) {
            KeySym modkeysym = xkeymap[mapindex + j];
            switch (modkeysym) {
                case XK_Num_Lock:
                    num_lock_mask |= maskbit;
                    break;
                case XK_Caps_Lock:
                    caps_lock_mask |= maskbit;
                    break;
                case XK_Shift_Lock:
                    shift_lock_mask |= maskbit;
                    break;
                case XK_Shift_L:
                case XK_Shift_R:
                    shift_mask |= maskbit;
                    break;
            }
        }
    }
    
    bool shift_state = ((shift_mask & aEvent->nativeModifiers()) != 0) ^
                          (bool)(shift_lock_mask & aEvent->nativeModifiers());
    bool capslock_state = (bool)(caps_lock_mask & aEvent->nativeModifiers());

    
    
    
    if (!domKeyCode &&
        aEvent->nativeScanCode() >= (quint32)x_min_keycode &&
        aEvent->nativeScanCode() <= (quint32)x_max_keycode) {
        int index = (aEvent->nativeScanCode() - x_min_keycode) * xkeysyms_per_keycode;
        for(int i = 0; (i < xkeysyms_per_keycode) && (domKeyCode == (quint32)NoSymbol); ++i) {
            domKeyCode = QtKeyCodeToDOMKeyCode(xkeymap[index + i]);
        }
    }

    
    if (aEvent->text().length() && aEvent->text()[0].isPrint())
        domCharCode = (int32_t) aEvent->text()[0].unicode();

    
    if (!aEvent->isAutoRepeat() && !IsKeyDown(domKeyCode)) {
        

        SetKeyDownFlag(domKeyCode);

        nsKeyEvent downEvent(true, NS_KEY_DOWN, this);
        InitKeyEvent(downEvent, aEvent);

        downEvent.keyCode = domKeyCode;

        nsEventStatus status = DispatchEvent(&downEvent);

        
        if (MOZ_UNLIKELY(mIsDestroyed)) {
            qWarning() << "Returning[" << __LINE__ << "]: " << "Window destroyed";
            return status;
        }

        
        if (status == nsEventStatus_eConsumeNoDefault)
            setNoDefault = true;
    }

    
    
    
    
    
    if (aEvent->key() == Qt::Key_Shift   ||
        aEvent->key() == Qt::Key_Control ||
        aEvent->key() == Qt::Key_Meta    ||
        aEvent->key() == Qt::Key_Alt     ||
        aEvent->key() == Qt::Key_AltGr) {

        return setNoDefault ?
            nsEventStatus_eConsumeNoDefault :
            nsEventStatus_eIgnore;
    }

    
    switch (aEvent->key()) {
        case Qt::Key_Back:
            return DispatchCommandEvent(nsGkAtoms::Back);
        case Qt::Key_Forward:
            return DispatchCommandEvent(nsGkAtoms::Forward);
        case Qt::Key_Refresh:
            return DispatchCommandEvent(nsGkAtoms::Reload);
        case Qt::Key_Stop:
            return DispatchCommandEvent(nsGkAtoms::Stop);
        case Qt::Key_Search:
            return DispatchCommandEvent(nsGkAtoms::Search);
        case Qt::Key_Favorites:
            return DispatchCommandEvent(nsGkAtoms::Bookmarks);
        case Qt::Key_HomePage:
            return DispatchCommandEvent(nsGkAtoms::Home);
        case Qt::Key_Copy:
        case Qt::Key_F16: 
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_COPY);
        case Qt::Key_Cut:
        case Qt::Key_F20:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_CUT);
        case Qt::Key_Paste:
        case Qt::Key_F18:
        case Qt::Key_F9:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_PASTE);
        case Qt::Key_F14:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_UNDO);
    }

    
    if (aEvent->nativeVirtualKey() == 0xff66) {
        return DispatchContentCommandEvent(NS_CONTENT_COMMAND_REDO);
    }
    if (aEvent->nativeVirtualKey() == 0xff65) {
        return DispatchContentCommandEvent(NS_CONTENT_COMMAND_UNDO);
    }

    nsKeyEvent event(true, NS_KEY_PRESS, this);
    InitKeyEvent(event, aEvent);

    
    if (setNoDefault) {
        event.mFlags.mDefaultPrevented = true;
    }

    
    
    
    
    if ((!domCharCode) &&
        (QApplication::keyboardModifiers() &
        (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {

        
        KeySym keysym = aEvent->nativeVirtualKey();
        if (keysym) {
            domCharCode = (uint32_t) keysym2ucs(keysym);
            if (domCharCode == -1 || !QChar((quint32)domCharCode).isPrint()) {
                domCharCode = 0;
            }
        }

        
        if (domCharCode > 0xFF && (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
            
            int index = (aEvent->nativeScanCode() - x_min_keycode) * xkeysyms_per_keycode;
            for (int i = 0; i < xkeysyms_per_keycode; ++i) {
                if (xkeymap[index + i] <= 0xFF && !shift_state) {
                    domCharCode = (uint32_t) QChar::toLower((uint) xkeymap[index + i]);
                    break;
                }
            }
        }

    } else { 
             
             
             
        event.modifiers &= ~(widget::MODIFIER_CONTROL |
                             widget::MODIFIER_ALT |
                             widget::MODIFIER_META);
    }

    KeySym keysym = NoSymbol;
    int index = (aEvent->nativeScanCode() - x_min_keycode) * xkeysyms_per_keycode;
    for (int i = 0; i < xkeysyms_per_keycode; ++i) {
        if (xkeymap[index + i] == aEvent->nativeVirtualKey()) {
            if ((i % 2) == 0) { 
                keysym = xkeymap[index + i + 1];
                break;
            } else { 
                keysym = xkeymap[index + i - 1];
                break;
            }
        }
        if (xkeysyms_per_keycode - 1 == i) {
            qWarning() << "Symbol '" << aEvent->nativeVirtualKey() << "' not found";
        }
    }
    QChar unshiftedChar(domCharCode);
    long ucs = keysym2ucs(keysym);
    ucs = ucs == -1 ? 0 : ucs;
    QChar shiftedChar((uint)ucs);

    
    
    
    if (domCharCode &&
        (QApplication::keyboardModifiers() &
        (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {

        event.charCode = domCharCode;
        event.keyCode = 0;
        nsAlternativeCharCode altCharCode(0, 0);
        
        if ((unshiftedChar.isUpper() || unshiftedChar.isLower()) &&
            unshiftedChar.toLower() == shiftedChar.toLower()) {
            if (shift_state ^ capslock_state) {
                altCharCode.mUnshiftedCharCode = (uint32_t) QChar::toUpper((uint)domCharCode);
                altCharCode.mShiftedCharCode = (uint32_t) QChar::toLower((uint)domCharCode);
            } else {
                altCharCode.mUnshiftedCharCode = (uint32_t) QChar::toLower((uint)domCharCode);
                altCharCode.mShiftedCharCode = (uint32_t) QChar::toUpper((uint)domCharCode);
            }
        } else {
            altCharCode.mUnshiftedCharCode = (uint32_t) unshiftedChar.unicode();
            altCharCode.mShiftedCharCode = (uint32_t) shiftedChar.unicode();
        }

        
        if ((altCharCode.mUnshiftedCharCode && altCharCode.mUnshiftedCharCode != domCharCode) ||
            (altCharCode.mShiftedCharCode && altCharCode.mShiftedCharCode != domCharCode)) {
            event.alternativeCharCodes.AppendElement(altCharCode);
        }

        
        if (altCharCode.mUnshiftedCharCode > 0xFF || altCharCode.mShiftedCharCode > 0xFF) {
            altCharCode.mUnshiftedCharCode = altCharCode.mShiftedCharCode = 0;

            
            KeySym keysym = NoSymbol;
            int index = (aEvent->nativeScanCode() - x_min_keycode) * xkeysyms_per_keycode;
            
            for (int i = 0; i < xkeysyms_per_keycode; ++i) {
                keysym = xkeymap[index + i];
                if (keysym && keysym <= 0xFF) {
                    if ((shift_state && (i % 2 == 1)) ||
                        (!shift_state && (i % 2 == 0))) {
                        altCharCode.mUnshiftedCharCode = altCharCode.mUnshiftedCharCode ?
                            altCharCode.mUnshiftedCharCode :
                            keysym;
                    } else {
                        altCharCode.mShiftedCharCode = altCharCode.mShiftedCharCode ?
                            altCharCode.mShiftedCharCode :
                            keysym;
                    }
                    if (altCharCode.mUnshiftedCharCode && altCharCode.mShiftedCharCode) {
                        break;
                    }
                }
            }

            if (altCharCode.mUnshiftedCharCode || altCharCode.mShiftedCharCode) {
                event.alternativeCharCodes.AppendElement(altCharCode);
            }
        }
    } else {
        event.charCode = domCharCode;
    }

    if (xmodmap) {
        XFreeModifiermap(xmodmap);
    }
    if (xkeymap) {
        XFree(xkeymap);
    }

    event.keyCode = domCharCode ? 0 : domKeyCode;
    
    return DispatchEvent(&event);
#else

    
    

    
    
    if (isContextMenuKeyEvent(aEvent)) {
        nsMouseEvent contextMenuEvent(true, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal,
                                      nsMouseEvent::eContextMenuKey);
        
        return DispatchEvent(&contextMenuEvent);
    }

    uint32_t domCharCode = 0;
    uint32_t domKeyCode = QtKeyCodeToDOMKeyCode(aEvent->key());

    if (aEvent->text().length() && aEvent->text()[0].isPrint())
        domCharCode = (int32_t) aEvent->text()[0].unicode();

    
    if (!aEvent->isAutoRepeat() && !IsKeyDown(domKeyCode)) {
        

        SetKeyDownFlag(domKeyCode);

        nsKeyEvent downEvent(true, NS_KEY_DOWN, this);
        InitKeyEvent(downEvent, aEvent);

        downEvent.keyCode = domKeyCode;

        nsEventStatus status = DispatchEvent(&downEvent);

        
        if (status == nsEventStatus_eConsumeNoDefault)
            setNoDefault = true;
    }

    nsKeyEvent event(true, NS_KEY_PRESS, this);
    InitKeyEvent(event, aEvent);

    event.charCode = domCharCode;

    event.keyCode = domCharCode ? 0 : domKeyCode;

    if (setNoDefault)
        event.mFlags.mDefaultPrevented = true;

    
    return DispatchEvent(&event);
#endif
}

nsEventStatus
nsWindow::OnKeyReleaseEvent(QKeyEvent *aEvent)
{
    LOGFOCUS(("OnKeyReleaseEvent [%p]\n", (void *)this));

    
    UserActivity();

    if (isContextMenuKeyEvent(aEvent)) {
        
        return nsEventStatus_eConsumeDoDefault;
    }

    uint32_t domKeyCode = QtKeyCodeToDOMKeyCode(aEvent->key());

#ifdef MOZ_X11
    if (!domKeyCode) {
        
        Display *display = mozilla::DefaultXDisplay();
        int x_min_keycode = 0, x_max_keycode = 0, xkeysyms_per_keycode;
        XDisplayKeycodes(display, &x_min_keycode, &x_max_keycode);
        KeySym *xkeymap = XGetKeyboardMapping(display, x_min_keycode, x_max_keycode - x_min_keycode,
                                              &xkeysyms_per_keycode);

        if (aEvent->nativeScanCode() >= (quint32)x_min_keycode &&
            aEvent->nativeScanCode() <= (quint32)x_max_keycode) {
            int index = (aEvent->nativeScanCode() - x_min_keycode) * xkeysyms_per_keycode;
            for(int i = 0; (i < xkeysyms_per_keycode) && (domKeyCode == (quint32)NoSymbol); ++i) {
                domKeyCode = QtKeyCodeToDOMKeyCode(xkeymap[index + i]);
            }
        }

        if (xkeymap) {
            XFree(xkeymap);
        }
    }
#endif 

    
    nsKeyEvent event(true, NS_KEY_UP, this);
    InitKeyEvent(event, aEvent);

    if (aEvent->key() == Qt::Key_AltGr) {
        sAltGrModifier = false;
    }

    event.keyCode = domKeyCode;

    
    ClearKeyDownFlag(event.keyCode);

    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnScrollEvent(QGraphicsSceneWheelEvent *aEvent)
{
    
    WheelEvent wheelEvent(true, NS_WHEEL_WHEEL, this);
    wheelEvent.deltaMode = nsIDOMWheelEvent::DOM_DELTA_LINE;

    
    
    
    
    
    int32_t delta = (int)(aEvent->delta() / WHEEL_DELTA) * -3;

    switch (aEvent->orientation()) {
    case Qt::Vertical:
        wheelEvent.deltaY = wheelEvent.lineOrPageDeltaY = delta;
        break;
    case Qt::Horizontal:
        wheelEvent.deltaX = wheelEvent.lineOrPageDeltaX = delta;
        break;
    default:
        Q_ASSERT(0);
        break;
    }

    wheelEvent.refPoint.x = nscoord(aEvent->scenePos().x());
    wheelEvent.refPoint.y = nscoord(aEvent->scenePos().y());

    wheelEvent.InitBasicModifiers(aEvent->modifiers() & Qt::ControlModifier,
                                  aEvent->modifiers() & Qt::AltModifier,
                                  aEvent->modifiers() & Qt::ShiftModifier,
                                  aEvent->modifiers() & Qt::MetaModifier);
    wheelEvent.time = 0;

    return DispatchEvent(&wheelEvent);
}


nsEventStatus
nsWindow::showEvent(QShowEvent *)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__,(void *)this));
    mIsVisible = true;
    return nsEventStatus_eConsumeDoDefault;
}

nsEventStatus
nsWindow::hideEvent(QHideEvent *)
{
    LOG(("%s [%p]\n", __PRETTY_FUNCTION__,(void *)this));
    mIsVisible = false;
    return nsEventStatus_eConsumeDoDefault;
}


#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
nsEventStatus nsWindow::OnTouchEvent(QTouchEvent *event, bool &handled)
{
    handled = false;
    const QList<QTouchEvent::TouchPoint> &touchPoints = event->touchPoints();

    if (event->type() == QEvent::TouchBegin) {
        handled = true;
        for (int i = touchPoints.count() -1; i >= 0; i--) {
            QPointF fpos = touchPoints[i].pos();
            nsGestureNotifyEvent gestureNotifyEvent(true, NS_GESTURENOTIFY_EVENT_START, this);
            gestureNotifyEvent.refPoint = nsIntPoint(fpos.x(), fpos.y());
            DispatchEvent(&gestureNotifyEvent);
        }
    }
    else if (event->type() == QEvent::TouchEnd) {
        mGesturesCancelled = false;
        mPinchEvent.needDispatch = false;
    }

    if (touchPoints.count() > 0) {
        
        
        mPinchEvent.touchPoint = touchPoints.at(0).pos();
    }

    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnGestureEvent(QGestureEvent* event, bool &handled) {

    handled = false;
    if (mGesturesCancelled) {
        return nsEventStatus_eIgnore;
    }

    nsEventStatus result = nsEventStatus_eIgnore;

    QGesture* gesture = event->gesture(Qt::PinchGesture);

    if (gesture) {
        QPinchGesture* pinch = static_cast<QPinchGesture*>(gesture);
        handled = true;

        mPinchEvent.centerPoint =
            mWidget->mapFromScene(event->mapToGraphicsScene(pinch->centerPoint()));
        nsIntPoint centerPoint(mPinchEvent.centerPoint.x(),
                               mPinchEvent.centerPoint.y());

        if (pinch->state() == Qt::GestureStarted) {
            event->accept();
            mPinchEvent.startDistance = DistanceBetweenPoints(mPinchEvent.centerPoint, mPinchEvent.touchPoint) * 2;
            mPinchEvent.prevDistance = mPinchEvent.startDistance;
            result = DispatchGestureEvent(NS_SIMPLE_GESTURE_MAGNIFY_START,
                                          0, 0, centerPoint);
        }
        else if (pinch->state() == Qt::GestureUpdated) {
            mPinchEvent.needDispatch = true;
            mPinchEvent.delta = 0;
            DispatchMotionToMainThread();
        }
        else if (pinch->state() == Qt::GestureFinished) {
            double distance = DistanceBetweenPoints(mPinchEvent.centerPoint, mPinchEvent.touchPoint) * 2;
            double delta = distance - mPinchEvent.startDistance;
            result = DispatchGestureEvent(NS_SIMPLE_GESTURE_MAGNIFY,
                                          0, delta, centerPoint);
            mPinchEvent.needDispatch = false;
        }
        else {
            handled = false;
        }

        
        
        mLastMultiTouchTime.start();
    }

    gesture = event->gesture(gSwipeGestureId);
    if (gesture) {
        if (gesture->state() == Qt::GestureStarted) {
            event->accept();
        }
        if (gesture->state() == Qt::GestureFinished) {
            event->accept();
            handled = true;

            MozSwipeGesture* swipe = static_cast<MozSwipeGesture*>(gesture);
            nsIntPoint hotspot;
            hotspot.x = swipe->hotSpot().x();
            hotspot.y = swipe->hotSpot().y();

            
            mGesturesCancelled = true;
            mPinchEvent.needDispatch = false;

            double distance = DistanceBetweenPoints(swipe->hotSpot(), mPinchEvent.touchPoint) * 2;
            double delta = distance - mPinchEvent.startDistance;

            DispatchGestureEvent(NS_SIMPLE_GESTURE_MAGNIFY, 0, delta / 2, hotspot);

            result = DispatchGestureEvent(NS_SIMPLE_GESTURE_SWIPE,
                                          swipe->Direction(), 0, hotspot);
        }
        mLastMultiTouchTime.start();
    }

    return result;
}

nsEventStatus
nsWindow::DispatchGestureEvent(uint32_t aMsg, uint32_t aDirection,
                               double aDelta, const nsIntPoint& aRefPoint)
{
    nsSimpleGestureEvent mozGesture(true, aMsg, this, 0, 0.0);
    mozGesture.direction = aDirection;
    mozGesture.delta = aDelta;
    mozGesture.refPoint = aRefPoint;

    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

    mozGesture.InitBasicModifiers(modifiers & Qt::ControlModifier,
                                  modifiers & Qt::AltModifier,
                                  modifiers & Qt::ShiftModifier,
                                  false);
    mozGesture.button    = 0;
    mozGesture.time      = 0;

    return DispatchEvent(&mozGesture);
}


double
nsWindow::DistanceBetweenPoints(const QPointF &aFirstPoint, const QPointF &aSecondPoint)
{
    double result = 0;
    double deltaX = abs(aFirstPoint.x() - aSecondPoint.x());
    double deltaY = abs(aFirstPoint.y() - aSecondPoint.y());
    result = sqrt(deltaX*deltaX + deltaY*deltaY);
    return result;
}

#endif 

void
nsWindow::ThemeChanged()
{
    NotifyThemeChanged();
}

nsEventStatus
nsWindow::OnDragMotionEvent(QGraphicsSceneDragDropEvent *aEvent)
{
    LOG(("nsWindow::OnDragMotionSignal\n"));

    nsMouseEvent event(true, NS_DRAGDROP_OVER, 0,
                       nsMouseEvent::eReal);
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnDragLeaveEvent(QGraphicsSceneDragDropEvent *aEvent)
{
    
    LOG(("nsWindow::OnDragLeaveSignal(%p)\n", this));
    nsMouseEvent event(true, NS_DRAGDROP_EXIT, this, nsMouseEvent::eReal);

    return DispatchEvent(&event);
}

nsEventStatus
nsWindow::OnDragDropEvent(QGraphicsSceneDragDropEvent *aDropEvent)
{
    if (aDropEvent->proposedAction() == Qt::CopyAction)
    {
        printf("text version of the data: %s\n", aDropEvent->mimeData()->text().toUtf8().data());
        aDropEvent->acceptProposedAction();
    }

    LOG(("nsWindow::OnDragDropSignal\n"));
    nsMouseEvent event(true, NS_DRAGDROP_OVER, 0,
                       nsMouseEvent::eReal);
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::OnDragEnter(QGraphicsSceneDragDropEvent *aDragEvent)
{
    
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

    nsMouseEvent event(true, NS_DRAGDROP_ENTER, this, nsMouseEvent::eReal);
    return DispatchEvent(&event);
}

static void
GetBrandName(nsXPIDLString& brandName)
{
    nsCOMPtr<nsIStringBundleService> bundleService =
        mozilla::services::GetStringBundleService();

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
nsWindow::Create(nsIWidget        *aParent,
                 nsNativeWidget    aNativeParent,
                 const nsIntRect  &aRect,
                 nsDeviceContext *aContext,
                 nsWidgetInitData *aInitData)
{
    
    
    nsIWidget *baseParent = aParent;

    if (aInitData &&
        (aInitData->mWindowType == eWindowType_dialog ||
         aInitData->mWindowType == eWindowType_toplevel ||
         aInitData->mWindowType == eWindowType_invisible)) {

        baseParent = nullptr;
        
        aNativeParent = nullptr;
    }

    
    BaseCreate(baseParent, aRect, aContext, aInitData);

    
    mParent = aParent;

    
    mBounds = aRect;

    
    MozQWidget *parent = nullptr;

    if (aParent != nullptr)
        parent = static_cast<MozQWidget*>(aParent->GetNativeData(NS_NATIVE_WIDGET));

    
    mWidget = createQWidget(parent, aNativeParent, aInitData);

    if (!mWidget)
        return NS_ERROR_OUT_OF_MEMORY;

    LOG(("Create: nsWindow [%p] [%p]\n", (void *)this, (void *)mWidget));

    
    Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, false);

    
    mListenForResizes = (aNativeParent ||
                         (aInitData && aInitData->mListenForResizes));

    return NS_OK;
}

already_AddRefed<nsIWidget>
nsWindow::CreateChild(const nsIntRect&  aRect,
                      nsDeviceContext* aContext,
                      nsWidgetInitData* aInitData,
                      bool              )
{
    
    return nsBaseWidget::CreateChild(aRect,
                                     aContext,
                                     aInitData,
                                     true); 
}


NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString &xulWinType)
{
    if (!mWidget)
      return NS_ERROR_FAILURE;

    nsXPIDLString brandName;
    GetBrandName(brandName);

#ifdef MOZ_X11
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

    QWidget *widget = GetViewWidget();
    
    if (widget && widget->winId())
        XSetClassHint(gfxQtPlatform::GetXDisplay(widget),
                      widget->winId(),
                      class_hint);

    nsMemory::Free(class_hint->res_class);
    nsMemory::Free(class_hint->res_name);
    XFree(class_hint);
#endif

    return NS_OK;
}

void
nsWindow::NativeResize(int32_t aWidth, int32_t aHeight, bool    aRepaint)
{
    LOG(("nsWindow::NativeResize [%p] %d %d\n", (void *)this,
         aWidth, aHeight));

    mNeedsResize = false;

    if (mIsTopLevel) {
        QGraphicsView *widget = qobject_cast<QGraphicsView*>(GetViewWidget());
        NS_ENSURE_TRUE_VOID(widget);
        
        QRect r = widget->mapFromScene(mWidget->mapToScene(QRect(0, 0, aWidth, aHeight))).boundingRect();
        
        r.adjust(0, 0, -1, -1);
        widget->resize(r.width(), r.height());
    }
    else {
        mWidget->resize(aWidth, aHeight);
    }

    if (aRepaint)
        mWidget->update();
}

void
nsWindow::NativeResize(int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       bool    aRepaint)
{
    LOG(("nsWindow::NativeResize [%p] %d %d %d %d\n", (void *)this,
         aX, aY, aWidth, aHeight));

    mNeedsResize = false;
    mNeedsMove = false;

    if (mIsTopLevel) {
        QGraphicsView *widget = qobject_cast<QGraphicsView*>(GetViewWidget());
        NS_ENSURE_TRUE_VOID(widget);
        
        QRect r = widget->mapFromScene(mWidget->mapToScene(QRect(aX, aY, aWidth, aHeight))).boundingRect();
        
        r.adjust(0, 0, -1, -1);
        widget->setGeometry(r.x(), r.y(), r.width(), r.height());
    }
    else {
        mWidget->setGeometry(aX, aY, aWidth, aHeight);
    }

    if (aRepaint)
        mWidget->update();
}

void
nsWindow::NativeShow(bool aAction)
{
    if (aAction) {
        QWidget *widget = GetViewWidget();
        
        
        
        if (widget &&
            !widget->isVisible())
            MakeFullScreen(mSizeMode == nsSizeMode_Fullscreen);
        mWidget->show();

        
        mNeedsShow = false;
    }
    else
        mWidget->hide();
}

NS_IMETHODIMP
nsWindow::SetHasTransparentBackground(bool aTransparent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::GetHasTransparentBackground(bool& aTransparent)
{
    aTransparent = mIsTransparent;
    return NS_OK;
}

void *
nsWindow::SetupPluginPort(void)
{
    NS_WARNING("Not implemented");
    return nullptr;
}

nsresult
nsWindow::SetWindowIconList(const nsTArray<nsCString> &aIconList)
{
    QIcon icon;

    for (uint32_t i = 0; i < aIconList.Length(); ++i) {
        const char *path = aIconList[i].get();
        LOG(("window [%p] Loading icon from %s\n", (void *)this, path));
        icon.addFile(path);
    }

    QWidget *widget = GetViewWidget();
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);
    widget->setWindowIcon(icon);

    return NS_OK;
}

void
nsWindow::SetDefaultIcon(void)
{
    SetIcon(NS_LITERAL_STRING("default"));
}

void nsWindow::QWidgetDestroyed()
{
    mWidget = nullptr;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(bool aFullScreen)
{
    QWidget *widget = GetViewWidget();
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);
    if (aFullScreen) {
        if (mSizeMode != nsSizeMode_Fullscreen)
            mLastSizeMode = mSizeMode;

        mSizeMode = nsSizeMode_Fullscreen;
        widget->showFullScreen();
    }
    else {
        mSizeMode = mLastSizeMode;

        switch (mSizeMode) {
        case nsSizeMode_Maximized:
            widget->showMaximized();
            break;
        case nsSizeMode_Minimized:
            widget->showMinimized();
            break;
        case nsSizeMode_Normal:
            widget->showNormal();
            break;
        default:
            widget->showNormal();
            break;
        }
    }

    NS_ASSERTION(mLastSizeMode != nsSizeMode_Fullscreen,
                 "mLastSizeMode should never be fullscreen");
    return nsBaseWidget::MakeFullScreen(aFullScreen);
}

NS_IMETHODIMP
nsWindow::HideWindowChrome(bool aShouldHide)
{
    if (!mWidget) {
        
        return NS_ERROR_FAILURE;
    }

    
    
    
    bool wasVisible = false;
    if (mWidget->isVisible()) {
        NativeShow(false);
        wasVisible = true;
    }

    if (wasVisible) {
        NativeShow(true);
    }

    
    
    
    
    
    QWidget *widget = GetViewWidget();
    NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);
#ifdef MOZ_X11
    XSync(gfxQtPlatform::GetXDisplay(widget), False);
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
    
    return NS_OK;
}

inline bool
is_context_menu_key(const nsKeyEvent& aKeyEvent)
{
    return ((aKeyEvent.keyCode == NS_VK_F10 && aKeyEvent.IsShift() &&
             !aKeyEvent.IsControl() && !aKeyEvent.IsMeta() &&
             !aKeyEvent.IsAlt()) ||
            (aKeyEvent.keyCode == NS_VK_CONTEXT_MENU && !aKeyEvent.IsShift() &&
             !aKeyEvent.IsControl() && !aKeyEvent.IsMeta() &&
             !aKeyEvent.IsAlt()));
}

void
key_event_to_context_menu_event(nsMouseEvent &aEvent,
                                QKeyEvent *aGdkEvent)
{
    aEvent.refPoint = nsIntPoint(0, 0);
    aEvent.modifiers = 0;
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
#ifdef DEBUG_WIDGETS
    qDebug("===================== popup!");
#endif
}

nsPopupWindow::~nsPopupWindow()
{
}

NS_IMETHODIMP_(bool)
nsWindow::HasGLContext()
{
    return MozQGLWidgetWrapper::hasGLContext(qobject_cast<QGraphicsView*>(GetViewWidget()));
}

MozQWidget*
nsWindow::createQWidget(MozQWidget *parent,
                        nsNativeWidget nativeParent,
                        nsWidgetInitData *aInitData)
{
    const char *windowName = NULL;
    Qt::WindowFlags flags = Qt::Widget;
    QWidget *parentWidget = (parent && parent->getReceiver()) ?
            parent->getReceiver()->GetViewWidget() : nullptr;

#ifdef DEBUG_WIDGETS
    qDebug("NEW WIDGET\n\tparent is %p (%s)", (void*)parent,
           parent ? qPrintable(parent->objectName()) : "null");
#endif

    
    switch (mWindowType) {
    case eWindowType_dialog:
        windowName = "topLevelDialog";
        mIsTopLevel = true;
        flags |= Qt::Dialog;
        break;
    case eWindowType_popup:
        windowName = "topLevelPopup";
        break;
    case eWindowType_toplevel:
        windowName = "topLevelWindow";
        mIsTopLevel = true;
        break;
    case eWindowType_invisible:
        windowName = "topLevelInvisible";
        break;
    case eWindowType_child:
    case eWindowType_plugin:
    default: 
        windowName = "paintArea";
        break;
    }

    MozQWidget* parentQWidget = nullptr;
    if (parent) {
        parentQWidget = parent;
    } else if (nativeParent && nativeParent != PARENTLESS_WIDGET) {
        parentQWidget = static_cast<MozQWidget*>(nativeParent);
    }
    MozQWidget * widget = new MozQWidget(this, parentQWidget);
    if (!widget)
        return nullptr;
    widget->setObjectName(QString(windowName));

    
    if (eWindowType_child == mWindowType || eWindowType_plugin == mWindowType) {
        widget->setFlag(QGraphicsItem::ItemIsFocusable);
        widget->setFocusPolicy(Qt::WheelFocus);
    }

    

    if (mIsTopLevel) {
        QGraphicsView* newView =
            nsFastStartup::GetStartupGraphicsView(parentWidget, widget);

        if (mWindowType == eWindowType_dialog) {
            newView->setWindowModality(Qt::WindowModal);
        }

#if defined(MOZ_PLATFORM_MAEMO) || defined(MOZ_GL_PROVIDER)
        if (ComputeShouldAccelerate(mUseLayersAcceleration)) {
            
            if (!HasGLContext()) {
                MozQGraphicsView *qview = qobject_cast<MozQGraphicsView*>(newView);
                if (qview) {
                    qview->setGLWidgetEnabled(true);
                }
            }
        }
#endif

        if (gfxQtPlatform::GetPlatform()->GetRenderMode() == gfxQtPlatform::RENDER_DIRECT) {
            
#if defined(MOZ_X11) && (QT_VERSION < QT_VERSION_CHECK(5,0,0))
            newView->viewport()->setAttribute(Qt::WA_PaintOnScreen, true);
#endif
            newView->viewport()->setAttribute(Qt::WA_NoSystemBackground, true);
        }
        
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
#if defined MOZ_ENABLE_MEEGOTOUCH
        
        newView->viewport()->ungrabGesture(Qt::PanGesture);
        newView->viewport()->ungrabGesture(Qt::TapGesture);
        newView->viewport()->ungrabGesture(Qt::TapAndHoldGesture);
        newView->viewport()->ungrabGesture(Qt::SwipeGesture);
#endif

        
        newView->viewport()->grabGesture(Qt::PinchGesture);
        newView->viewport()->grabGesture(gSwipeGestureId);
#endif
        newView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        newView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
        
        widget->setFlag(QGraphicsItem::ItemHasNoContents);
#endif

#ifdef MOZ_X11
        if (newView->effectiveWinId()) {
            XSetWindowBackgroundPixmap(mozilla::DefaultXDisplay(),
                                       newView->effectiveWinId(), None);
        }
#endif
    }

    if (mWindowType == eWindowType_popup) {
        widget->setZValue(100);

        
        
        
        SetCursor(eCursor_standard);
    } else if (mIsTopLevel) {
        SetDefaultIcon();
    }
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
#if defined MOZ_ENABLE_MEEGOTOUCH
    
    widget->ungrabGesture(Qt::PanGesture);
    widget->ungrabGesture(Qt::TapGesture);
    widget->ungrabGesture(Qt::TapAndHoldGesture);
    widget->ungrabGesture(Qt::SwipeGesture);
#endif
    widget->grabGesture(Qt::PinchGesture);
    widget->grabGesture(gSwipeGestureId);
#endif

    return widget;
}


gfxASurface*
nsWindow::GetThebesSurface()
{
    


    if (mThebesSurface)
        return mThebesSurface;

#ifdef CAIRO_HAS_QT_SURFACE
    gfxQtPlatform::RenderMode renderMode = gfxQtPlatform::GetPlatform()->GetRenderMode();
    if (renderMode == gfxQtPlatform::RENDER_QPAINTER) {
        mThebesSurface = new gfxQPainterSurface(gfxIntSize(1, 1), gfxASurface::CONTENT_COLOR);
    }
#endif
    if (!mThebesSurface) {
        gfxASurface::gfxImageFormat imageFormat = gfxASurface::ImageFormatRGB24;
        mThebesSurface = new gfxImageSurface(gfxIntSize(1, 1), imageFormat);
    }

    return mThebesSurface;
}

NS_IMETHODIMP
nsWindow::BeginResizeDrag(nsGUIEvent* aEvent, int32_t aHorizontal, int32_t aVertical)
{
    NS_ENSURE_ARG_POINTER(aEvent);

    if (aEvent->eventStructType != NS_MOUSE_EVENT) {
        
        return NS_ERROR_INVALID_ARG;
    }

    nsMouseEvent* mouse_event = static_cast<nsMouseEvent*>(aEvent);

    if (mouse_event->button != nsMouseEvent::eLeftButton) {
        
        return NS_ERROR_INVALID_ARG;
    }

    return NS_OK;
}

nsEventStatus
nsWindow::contextMenuEvent(QGraphicsSceneContextMenuEvent *)
{
    return nsEventStatus_eIgnore;
}

nsEventStatus
nsWindow::imComposeEvent(QInputMethodEvent *event, bool &handled)
{
    
    

    nsCompositionEvent start(true, NS_COMPOSITION_START, this);
    DispatchEvent(&start);

    nsAutoString compositionStr(event->commitString().utf16());

    if (!compositionStr.IsEmpty()) {
      nsCompositionEvent update(true, NS_COMPOSITION_UPDATE, this);
      update.data = compositionStr;
      DispatchEvent(&update);
    }

    nsTextEvent text(true, NS_TEXT_TEXT, this);
    text.theText = compositionStr;
    DispatchEvent(&text);

    nsCompositionEvent end(true, NS_COMPOSITION_END, this);
    end.data = compositionStr;
    DispatchEvent(&end);

    return nsEventStatus_eIgnore;
}

nsIWidget *
nsWindow::GetParent(void)
{
    return mParent;
}

float
nsWindow::GetDPI()
{
    QDesktopWidget* rootWindow = QApplication::desktop();
    double heightInches = rootWindow->heightMM()/25.4;
    if (heightInches < 0.25) {
        
        return 96.0f;
    }

    return float(rootWindow->height()/heightInches);
}

void
nsWindow::DispatchActivateEvent(void)
{
    if (mWidgetListener)
      mWidgetListener->WindowActivated();
}

void
nsWindow::DispatchDeactivateEvent(void)
{
    if (mWidgetListener)
      mWidgetListener->WindowDeactivated();
}

void
nsWindow::DispatchActivateEventOnTopLevelWindow(void)
{
    nsWindow * topLevelWindow = static_cast<nsWindow*>(GetTopLevelWidget());
    if (topLevelWindow != nullptr)
         topLevelWindow->DispatchActivateEvent();
}

void
nsWindow::DispatchDeactivateEventOnTopLevelWindow(void)
{
    nsWindow * topLevelWindow = static_cast<nsWindow*>(GetTopLevelWidget());
    if (topLevelWindow != nullptr)
         topLevelWindow->DispatchDeactivateEvent();
}

void
nsWindow::DispatchResizeEvent(nsIntRect &aRect, nsEventStatus &aStatus)
{
    aStatus = nsEventStatus_eIgnore;
    if (mWidgetListener &&
        mWidgetListener->WindowResized(this, aRect.width, aRect.height))
      aStatus = nsEventStatus_eConsumeNoDefault;
}

NS_IMETHODIMP
nsWindow::DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus)
{
#ifdef DEBUG
    debug_DumpEvent(stdout, aEvent->widget, aEvent,
                    nsAutoCString("something"), 0);
#endif

    aStatus = nsEventStatus_eIgnore;

    
    if (mWidgetListener)
      aStatus = mWidgetListener->HandleEvent(aEvent, mUseAttachedEvents);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Show(bool aState)
{
    LOG(("nsWindow::Show [%p] state %d\n", (void *)this, aState));
    if (aState == mIsShown)
        return NS_OK;

    
    if (mIsShown && !aState) {
        ClearCachedResources();
    }

    mIsShown = aState;

#ifdef MOZ_ENABLE_QTMOBILITY
    if (mWidget &&
        (mWindowType == eWindowType_toplevel ||
         mWindowType == eWindowType_dialog ||
         mWindowType == eWindowType_popup))
    {
        if (!gOrientation) {
            gOrientation = new QOrientationSensor();
            gOrientation->addFilter(&gOrientationFilter);
            gOrientation->start();
            if (!gOrientation->isActive()) {
                qWarning("Orientationsensor didn't start!");
            }
            gOrientationFilter.filter(gOrientation->reading());

            QObject::connect((QObject*) &gOrientationFilter, SIGNAL(orientationChanged()),
                             mWidget, SLOT(orientationChanged()));
        }
    }
#endif

    if ((aState && !AreBoundsSane()) || !mWidget) {
        LOG(("\tbounds are insane or window hasn't been created yet\n"));
        mNeedsShow = true;
        return NS_OK;
    }

    if (aState) {
        if (mNeedsMove) {
            NativeResize(mBounds.x, mBounds.y, mBounds.width, mBounds.height,
                         false);
        } else if (mNeedsResize) {
            NativeResize(mBounds.width, mBounds.height, false);
        }
    }
    else
        
        mNeedsShow = false;

    NativeShow(aState);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aWidth, double aHeight, bool aRepaint)
{
    mBounds.width = NSToIntRound(aWidth);
    mBounds.height = NSToIntRound(aHeight);

    if (!mWidget)
        return NS_OK;

    if (mIsShown) {
        if (AreBoundsSane()) {
            if (mIsTopLevel || mNeedsShow)
                NativeResize(mBounds.x, mBounds.y,
                             mBounds.width, mBounds.height, aRepaint);
            else
                NativeResize(mBounds.width, mBounds.height, aRepaint);

            
            if (mNeedsShow)
                NativeShow(true);
        }
        else {
            
            
            
            
            
            
            if (!mNeedsShow) {
                mNeedsShow = true;
                NativeShow(false);
            }
        }
    }
    else if (AreBoundsSane() && mListenForResizes) {
        
        
        
        NativeResize(mBounds.width, mBounds.height, aRepaint);
    }
    else {
        mNeedsResize = true;
    }

    
    if (mIsTopLevel || mListenForResizes) {
        nsEventStatus status;
        DispatchResizeEvent(mBounds, status);
    }

    NotifyRollupGeometryChange();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aX, double aY, double aWidth, double aHeight,
                 bool aRepaint)
{
    mBounds.x = NSToIntRound(aX);
    mBounds.y = NSToIntRound(aY);
    mBounds.width = NSToIntRound(aWidth);
    mBounds.height = NSToIntRound(aHeight);

    mPlaced = true;

    if (!mWidget)
        return NS_OK;

    
    if (mIsShown) {
        
        if (AreBoundsSane()) {
            
            NativeResize(mBounds.x, mBounds.y, mBounds.width, mBounds.height,
                         aRepaint);
            
            if (mNeedsShow)
                NativeShow(true);
        }
        else {
            
            
            
            
            
            
            if (!mNeedsShow) {
                mNeedsShow = true;
                NativeShow(false);
            }
        }
    }
    
    
    else if (AreBoundsSane() && mListenForResizes) {
        
        
        
        NativeResize(mBounds.x, mBounds.y, mBounds.width, mBounds.height,
                     aRepaint);
    }
    else {
        mNeedsResize = true;
        mNeedsMove = true;
    }

    if (mIsTopLevel || mListenForResizes) {
        
        nsEventStatus status;
        DispatchResizeEvent(mBounds, status);
    }

    if (aRepaint)
        mWidget->update();

    NotifyRollupGeometryChange();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Enable(bool aState)
{
    mEnabled = aState;

    return NS_OK;
}

bool
nsWindow::IsEnabled() const
{
    return mEnabled;
}

void
nsWindow::OnDestroy(void)
{
    if (mOnDestroyCalled)
        return;

    mOnDestroyCalled = true;

    
    nsBaseWidget::OnDestroy();

    
    mParent = nullptr;

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;
    NotifyWindowDestroyed();
}

bool
nsWindow::AreBoundsSane(void)
{
    if (mBounds.width > 0 && mBounds.height > 0)
        return true;

    return false;
}

#if defined(MOZ_X11) && (MOZ_PLATFORM_MAEMO == 6)
typedef enum {
    VKBUndefined,
    VKBOpen,
    VKBClose
} PluginVKBState;

static QCoreApplication::EventFilter previousEventFilter = NULL;

static PluginVKBState
GetPluginVKBState(Window aWinId)
{
    
    PluginVKBState imeState = VKBUndefined;
    Display *display = mozilla::DefaultXDisplay();

    Atom actualType;
    int actualFormat;
    unsigned long nitems;
    unsigned long bytes;
    union {
        unsigned char* asUChar;
        unsigned long* asLong;
    } data = {0};
    int status = XGetWindowProperty(display, aWinId, sPluginIMEAtom,
                                    0, 1, False, AnyPropertyType,
                                    &actualType, &actualFormat, &nitems,
                                    &bytes, &data.asUChar);

    if (status == Success && actualType == XA_CARDINAL && actualFormat == 32 && nitems == 1) {
        
        imeState = data.asLong[0] ? VKBOpen : VKBClose;
    }

    if (status == Success) {
        XFree(data.asUChar);
    }

    return imeState;
}

static void
SetVKBState(Window aWinId, PluginVKBState aState)
{
    Display *display = mozilla::DefaultXDisplay();
    if (aState != VKBUndefined) {
        unsigned long isOpen = aState == VKBOpen ? 1 : 0;
        XChangeProperty(display, aWinId, sPluginIMEAtom, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) &isOpen, 1);
    } else {
        XDeleteProperty(display, aWinId, sPluginIMEAtom);
    }
    XSync(display, False);
}

static bool
x11EventFilter(void* message, long* result)
{
    XEvent* event = static_cast<XEvent*>(message);
    if (event->type == PropertyNotify) {
        if (event->xproperty.atom == sPluginIMEAtom) {
            PluginVKBState state = GetPluginVKBState(event->xproperty.window);
            if (state == VKBOpen) {
                MozQWidget::requestVKB();
            } else if (state == VKBClose) {
                MozQWidget::hideVKB();
            }
            return true;
        }
    }
    if (previousEventFilter) {
        return previousEventFilter(message, result);
    }

    return false;
}
#endif

NS_IMETHODIMP_(void)
nsWindow::SetInputContext(const InputContext& aContext,
                          const InputContextAction& aAction)
{
    NS_ENSURE_TRUE_VOID(mWidget);

    
    
    mInputContext = aContext;

#if defined(MOZ_X11) && (MOZ_PLATFORM_MAEMO == 6)
    if (sPluginIMEAtom) {
        static QCoreApplication::EventFilter currentEventFilter = NULL;
        if (mInputContext.mIMEState.mEnabled == IMEState::PLUGIN &&
            currentEventFilter != x11EventFilter) {
            
            previousEventFilter = QCoreApplication::instance()->setEventFilter(x11EventFilter);
            currentEventFilter = x11EventFilter;
        } else if (mInputContext.mIMEState.mEnabled != IMEState::PLUGIN &&
                   currentEventFilter == x11EventFilter) {
            
            QCoreApplication::instance()->setEventFilter(previousEventFilter);
            currentEventFilter = previousEventFilter;
            previousEventFilter = NULL;
            QWidget* view = GetViewWidget();
            if (view) {
                SetVKBState(view->winId(), VKBUndefined);
            }
        }
    }
#endif

    switch (mInputContext.mIMEState.mEnabled) {
        case IMEState::ENABLED:
        case IMEState::PASSWORD:
        case IMEState::PLUGIN:
            SetSoftwareKeyboardState(true, aAction);
            break;
        default:
            SetSoftwareKeyboardState(false, aAction);
            break;
    }
}

NS_IMETHODIMP_(InputContext)
nsWindow::GetInputContext()
{
    mInputContext.mIMEState.mOpen = IMEState::OPEN_STATE_NOT_SUPPORTED;
    
    
#if (QT_VERSION <= QT_VERSION_CHECK(5, 0, 0))
    mInputContext.mNativeIMEContext = qApp->inputContext();
#else
    mInputContext.mNativeIMEContext = nullptr;
#endif
    return mInputContext;
}

void
nsWindow::SetSoftwareKeyboardState(bool aOpen,
                                   const InputContextAction& aAction)
{
    if (aOpen) {
        NS_ENSURE_TRUE_VOID(mInputContext.mIMEState.mEnabled !=
                            IMEState::DISABLED);

        
        
        if (mInputContext.mIMEState.mEnabled != IMEState::PLUGIN &&
            Preferences::GetBool("content.ime.strict_policy", false) &&
            !aAction.ContentGotFocusByTrustedCause() &&
            !aAction.UserMightRequestOpenVKB()) {
            return;
        }
#if defined(MOZ_X11) && (MOZ_PLATFORM_MAEMO == 6)
        
        else if (sPluginIMEAtom) {
            QWidget* view = GetViewWidget();
            if (view && GetPluginVKBState(view->winId()) == VKBClose) {
                return;
            }
        }
#endif
    }

    if (aOpen) {
        
        
        int32_t openDelay =
            Preferences::GetInt("ui.vkb.open.delay", 200);
        MozQWidget::requestVKB(openDelay, mWidget);
    } else {
        MozQWidget::hideVKB();
    }
    return;
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
        mLayerManager->GetBackendType() == mozilla::layers::LAYERS_OPENGL) {
        return MozQGLWidgetWrapper::isRGBAContext() ? LOCAL_GL_RGBA : LOCAL_GL_RGB;
    }
    return LOCAL_GL_NONE;
}

