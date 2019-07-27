






#include "mozilla/ArrayUtils.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextEvents.h"
#include <algorithm>

#include "GeckoProfiler.h"

#include "prlink.h"
#include "nsGTKToolkit.h"
#include "nsIRollupListener.h"
#include "nsIDOMNode.h"

#include "nsWidgetsCID.h"
#include "nsDragService.h"
#include "nsIWidgetListener.h"
#include "nsIScreenManager.h"

#include "nsGtkKeyUtils.h"
#include "nsGtkCursors.h"

#include <gtk/gtk.h>
#if (MOZ_WIDGET_GTK == 3)
#include <gtk/gtkx.h>
#endif
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/shape.h>
#if (MOZ_WIDGET_GTK == 3)
#include <gdk/gdkkeysyms-compat.h>
#endif

#ifdef AIX
#include <X11/keysym.h>
#else
#include <X11/XF86keysym.h>
#endif

#if (MOZ_WIDGET_GTK == 2)
#include "gtk2xtbin.h"
#endif
#endif 
#include <gdk/gdkkeysyms.h>
#if (MOZ_WIDGET_GTK == 2)
#include <gtk/gtkprivate.h>
#endif

#include "nsGkAtoms.h"

#ifdef MOZ_ENABLE_STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <startup-notification-1.0/libsn/sn.h>
#endif

#include "mozilla/Assertions.h"
#include "mozilla/Likely.h"
#include "mozilla/Preferences.h"
#include "nsIPrefService.h"
#include "nsIGConfService.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsGfxCIID.h"
#include "nsGtkUtils.h"
#include "nsIObserverService.h"
#include "mozilla/layers/LayersTypes.h"
#include "nsIIdleServiceInternal.h"
#include "nsIPropertyBag2.h"
#include "GLContext.h"
#include "gfx2DGlue.h"
#include "nsPluginNativeWindowGtk.h"

#ifdef ACCESSIBILITY
#include "mozilla/a11y/Accessible.h"
#include "mozilla/a11y/Platform.h"
#include "nsAccessibilityService.h"

using namespace mozilla;
using namespace mozilla::widget;
#endif


#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"


#include <gdk/gdk.h>
#include <wchar.h>
#include "imgIContainer.h"
#include "nsGfxCIID.h"
#include "nsImageToPixbuf.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"
#include "ClientLayerManager.h"

extern "C" {
#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"
}
#include "gfxPlatformGtk.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "Layers.h"
#include "GLContextProvider.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/layers/CompositorParent.h"

#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#include "cairo-xlib.h"
#endif
  
#include "nsShmImage.h"

#include "nsIDOMWheelEvent.h"

#include "NativeKeyBindings.h"
#include "nsWindow.h"

#include <dlfcn.h>

#include "mozilla/layers/APZCTreeManager.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::widget;
using namespace mozilla::layers;
using mozilla::gl::GLContext;



#define MAX_RECTS_IN_REGION 100

const gint kEvents = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK |
                     GDK_VISIBILITY_NOTIFY_MASK |
                     GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                     GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
#if GTK_CHECK_VERSION(3,4,0)
                     GDK_SMOOTH_SCROLL_MASK |
#endif
                     GDK_SCROLL_MASK |
                     GDK_POINTER_MOTION_MASK;


static bool       is_mouse_in_window(GdkWindow* aWindow,
                                     gdouble aMouseX, gdouble aMouseY);
static nsWindow  *get_window_for_gtk_widget(GtkWidget *widget);
static nsWindow  *get_window_for_gdk_window(GdkWindow *window);
static GtkWidget *get_gtk_widget_for_gdk_window(GdkWindow *window);
static GdkCursor *get_gtk_cursor(nsCursor aCursor);

static GdkWindow *get_inner_gdk_window (GdkWindow *aWindow,
                                        gint x, gint y,
                                        gint *retx, gint *rety);

static inline bool is_context_menu_key(const WidgetKeyboardEvent& inKeyEvent);

static int    is_parent_ungrab_enter(GdkEventCrossing *aEvent);
static int    is_parent_grab_leave(GdkEventCrossing *aEvent);

static void GetBrandName(nsXPIDLString& brandName);


#if (MOZ_WIDGET_GTK == 2)
static gboolean expose_event_cb           (GtkWidget *widget,
                                           GdkEventExpose *event);
#else
static gboolean expose_event_cb           (GtkWidget *widget,
                                           cairo_t *rect);
#endif
static gboolean configure_event_cb        (GtkWidget *widget,
                                           GdkEventConfigure *event);
static void     container_unrealize_cb    (GtkWidget *widget);
static void     size_allocate_cb          (GtkWidget *widget,
                                           GtkAllocation *allocation);
static gboolean delete_event_cb           (GtkWidget *widget,
                                           GdkEventAny *event);
static gboolean enter_notify_event_cb     (GtkWidget *widget,
                                           GdkEventCrossing *event);
static gboolean leave_notify_event_cb     (GtkWidget *widget,
                                           GdkEventCrossing *event);
static gboolean motion_notify_event_cb    (GtkWidget *widget,
                                           GdkEventMotion *event);
static gboolean button_press_event_cb     (GtkWidget *widget,
                                           GdkEventButton *event);
static gboolean button_release_event_cb   (GtkWidget *widget,
                                           GdkEventButton *event);
static gboolean focus_in_event_cb         (GtkWidget *widget,
                                           GdkEventFocus *event);
static gboolean focus_out_event_cb        (GtkWidget *widget,
                                           GdkEventFocus *event);
static gboolean key_press_event_cb        (GtkWidget *widget,
                                           GdkEventKey *event);
static gboolean key_release_event_cb      (GtkWidget *widget,
                                           GdkEventKey *event);
static gboolean scroll_event_cb           (GtkWidget *widget,
                                           GdkEventScroll *event);
static gboolean visibility_notify_event_cb(GtkWidget *widget,
                                           GdkEventVisibility *event);
static void     hierarchy_changed_cb      (GtkWidget *widget,
                                           GtkWidget *previous_toplevel);
static gboolean window_state_event_cb     (GtkWidget *widget,
                                           GdkEventWindowState *event);
static void     theme_changed_cb          (GtkSettings *settings,
                                           GParamSpec *pspec,
                                           nsWindow *data);
static nsWindow* GetFirstNSWindowForGDKWindow (GdkWindow *aGdkWindow);

#ifdef __cplusplus
extern "C" {
#endif
#ifdef MOZ_X11
static GdkFilterReturn popup_take_focus_filter (GdkXEvent *gdk_xevent,
                                                GdkEvent *event,
                                                gpointer data);
static GdkFilterReturn plugin_window_filter_func (GdkXEvent *gdk_xevent,
                                                  GdkEvent *event,
                                                  gpointer data);
static GdkFilterReturn plugin_client_message_filter (GdkXEvent *xevent,
                                                     GdkEvent *event,
                                                     gpointer data);
#endif 
#ifdef __cplusplus
}
#endif 

static gboolean drag_motion_event_cb      (GtkWidget *aWidget,
                                           GdkDragContext *aDragContext,
                                           gint aX,
                                           gint aY,
                                           guint aTime,
                                           gpointer aData);
static void     drag_leave_event_cb       (GtkWidget *aWidget,
                                           GdkDragContext *aDragContext,
                                           guint aTime,
                                           gpointer aData);
static gboolean drag_drop_event_cb        (GtkWidget *aWidget,
                                           GdkDragContext *aDragContext,
                                           gint aX,
                                           gint aY,
                                           guint aTime,
                                           gpointer aData);
static void    drag_data_received_event_cb(GtkWidget *aWidget,
                                           GdkDragContext *aDragContext,
                                           gint aX,
                                           gint aY,
                                           GtkSelectionData  *aSelectionData,
                                           guint aInfo,
                                           guint32 aTime,
                                           gpointer aData);


static nsresult    initialize_prefs        (void);

static guint32 sLastUserInputTime = GDK_CURRENT_TIME;
static guint32 sRetryGrabTime;

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);


static nsWindow         *gFocusWindow          = nullptr;
static bool              gBlockActivateEvent   = false;
static bool              gGlobalsInitialized   = false;
static bool              gRaiseWindows         = true;
static nsWindow         *gPluginFocusWindow    = nullptr;


#define NS_WINDOW_TITLE_MAX_LENGTH 4095






typedef struct _GdkDisplay GdkDisplay;

#define kWindowPositionSlop 20


static GdkCursor *gCursorCache[eCursorCount];

static GtkWidget *gInvisibleContainer = nullptr;



static guint gButtonState;



template <>
class nsSimpleRef<pixman_region32> : public pixman_region32 {
protected:
    typedef pixman_region32 RawRef;

    nsSimpleRef() { data = nullptr; }
    explicit nsSimpleRef(const RawRef &aRawRef) : pixman_region32(aRawRef) { }

    static void Release(pixman_region32& region) {
        pixman_region32_fini(&region);
    }
    
    bool HaveResource() const { return data != nullptr; }

    pixman_region32& get() { return *this; }
};

static inline int32_t
GetBitmapStride(int32_t width)
{
#if defined(MOZ_X11) || (MOZ_WIDGET_GTK == 2)
  return (width+7)/8;
#else
  return cairo_format_stride_for_width(CAIRO_FORMAT_A1, width);
#endif
}

static inline bool TimestampIsNewerThan(guint32 a, guint32 b)
{
    
    
    return a - b <= G_MAXUINT32/2;
}

static void
UpdateLastInputEventTime(void *aGdkEvent)
{
    nsCOMPtr<nsIIdleServiceInternal> idleService =
        do_GetService("@mozilla.org/widget/idleservice;1");
    if (idleService) {
        idleService->ResetIdleTimeOut(0);
    }

    guint timestamp = gdk_event_get_time(static_cast<GdkEvent*>(aGdkEvent));
    if (timestamp == GDK_CURRENT_TIME)
        return;

    sLastUserInputTime = timestamp;
}

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsBaseWidget)

nsWindow::nsWindow()
{
    mIsTopLevel       = false;
    mIsDestroyed      = false;
    mNeedsResize      = false;
    mNeedsMove        = false;
    mListenForResizes = false;
    mIsShown          = false;
    mNeedsShow        = false;
    mEnabled          = true;
    mCreated          = false;

    mContainer           = nullptr;
    mGdkWindow           = nullptr;
    mShell               = nullptr;
    mPluginNativeWindow  = nullptr;
    mHasMappedToplevel   = false;
    mIsFullyObscured     = false;
    mRetryPointerGrab    = false;
    mWindowType          = eWindowType_child;
    mSizeState           = nsSizeMode_Normal;
    mLastSizeMode        = nsSizeMode_Normal;
    mSizeConstraints.mMaxSize = GetSafeWindowSize(mSizeConstraints.mMaxSize);

#ifdef MOZ_X11
    mOldFocusWindow      = 0;
#endif 
    mPluginType          = PluginType_NONE;

    if (!gGlobalsInitialized) {
        gGlobalsInitialized = true;

        
        initialize_prefs();
    }

    mLastMotionPressure = 0;

#ifdef ACCESSIBILITY
    mRootAccessible  = nullptr;
#endif

    mIsTransparent = false;
    mTransparencyBitmap = nullptr;

    mTransparencyBitmapWidth  = 0;
    mTransparencyBitmapHeight = 0;

#if GTK_CHECK_VERSION(3,4,0)
    mLastScrollEventTime = GDK_CURRENT_TIME;
#endif
}

nsWindow::~nsWindow()
{
    LOG(("nsWindow::~nsWindow() [%p]\n", (void *)this));

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = nullptr;

    Destroy();
}

 void
nsWindow::ReleaseGlobals()
{
  for (uint32_t i = 0; i < ArrayLength(gCursorCache); ++i) {
    if (gCursorCache[i]) {
#if (MOZ_WIDGET_GTK == 3)
      g_object_unref(gCursorCache[i]);
#else
      gdk_cursor_unref(gCursorCache[i]);
#endif
      gCursorCache[i] = nullptr;
    }
  }
}

void
nsWindow::CommonCreate(nsIWidget *aParent, bool aListenForResizes)
{
    mParent = aParent;
    mListenForResizes = aListenForResizes;
    mCreated = true;
}

void
nsWindow::DispatchActivateEvent(void)
{
    NS_ASSERTION(mContainer || mIsDestroyed,
                 "DispatchActivateEvent only intended for container windows");

#ifdef ACCESSIBILITY
    DispatchActivateEventAccessible();
#endif 

    if (mWidgetListener)
      mWidgetListener->WindowActivated();
}

void
nsWindow::DispatchDeactivateEvent(void)
{
    if (mWidgetListener)
      mWidgetListener->WindowDeactivated();

#ifdef ACCESSIBILITY
    DispatchDeactivateEventAccessible();
#endif 
}

void
nsWindow::DispatchResized(int32_t aWidth, int32_t aHeight)
{
    nsIWidgetListener *listeners[] =
        { mWidgetListener, mAttachedWidgetListener };
    for (size_t i = 0; i < ArrayLength(listeners); ++i) {
        if (listeners[i]) {
            listeners[i]->WindowResized(this, aWidth, aHeight);
        }
    }
}

nsresult
nsWindow::DispatchEvent(WidgetGUIEvent* aEvent, nsEventStatus& aStatus)
{
#ifdef DEBUG
    debug_DumpEvent(stdout, aEvent->widget, aEvent,
                    nsAutoCString("something"), 0);
#endif
    
    aEvent->refPoint.x = GdkCoordToDevicePixels(aEvent->refPoint.x);
    aEvent->refPoint.y = GdkCoordToDevicePixels(aEvent->refPoint.y);

    aStatus = nsEventStatus_eIgnore;
    nsIWidgetListener* listener =
        mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
    if (listener) {
      aStatus = listener->HandleEvent(aEvent, mUseAttachedEvents);
    }

    return NS_OK;
}

void
nsWindow::OnDestroy(void)
{
    if (mOnDestroyCalled)
        return;

    mOnDestroyCalled = true;
    
    
    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;

    
    nsBaseWidget::OnDestroy(); 
    
    
    nsBaseWidget::Destroy();
    mParent = nullptr;

    NotifyWindowDestroyed();
}

bool
nsWindow::AreBoundsSane(void)
{
    if (mBounds.width > 0 && mBounds.height > 0)
        return true;

    return false;
}

static GtkWidget*
EnsureInvisibleContainer()
{
    if (!gInvisibleContainer) {
        
        
        
        
        GtkWidget* window = gtk_window_new(GTK_WINDOW_POPUP);
        gInvisibleContainer = moz_container_new();
        gtk_container_add(GTK_CONTAINER(window), gInvisibleContainer);
        gtk_widget_realize(gInvisibleContainer);

    }
    return gInvisibleContainer;
}

static void
CheckDestroyInvisibleContainer()
{
    NS_PRECONDITION(gInvisibleContainer, "oh, no");

    if (!gdk_window_peek_children(gtk_widget_get_window(gInvisibleContainer))) {
        
        
        gtk_widget_destroy(gtk_widget_get_parent(gInvisibleContainer));
        gInvisibleContainer = nullptr;
    }
}




static void
SetWidgetForHierarchy(GdkWindow *aWindow,
                      GtkWidget *aOldWidget,
                      GtkWidget *aNewWidget)
{
    gpointer data;
    gdk_window_get_user_data(aWindow, &data);

    if (data != aOldWidget) {
        if (!GTK_IS_WIDGET(data))
            return;

        GtkWidget* widget = static_cast<GtkWidget*>(data);
        if (gtk_widget_get_parent(widget) != aOldWidget)
            return;

        
        
        gtk_widget_reparent(widget, aNewWidget);

        return;
    }

    GList *children = gdk_window_get_children(aWindow);
    for(GList *list = children; list; list = list->next) {
        SetWidgetForHierarchy(GDK_WINDOW(list->data), aOldWidget, aNewWidget);
    }
    g_list_free(children);

    gdk_window_set_user_data(aWindow, aNewWidget);
}


void
nsWindow::DestroyChildWindows()
{
    if (!mGdkWindow)
        return;

    while (GList *children = gdk_window_peek_children(mGdkWindow)) {
        GdkWindow *child = GDK_WINDOW(children->data);
        nsWindow *kid = get_window_for_gdk_window(child);
        if (kid) {
            kid->Destroy();
        } else {
            
            
            gpointer data;
            gdk_window_get_user_data(child, &data);
            if (GTK_IS_WIDGET(data)) {
                gtk_widget_destroy(static_cast<GtkWidget*>(data));
            }
        }
    }
}

NS_IMETHODIMP
nsWindow::Destroy(void)
{
    if (mIsDestroyed || !mCreated)
        return NS_OK;

    LOG(("nsWindow::Destroy [%p]\n", (void *)this));
    mIsDestroyed = true;
    mCreated = false;

    
    if (mLayerManager) {
        mLayerManager->Destroy();
    }
    mLayerManager = nullptr;

    
    
    
    
    
    DestroyCompositor();

    ClearCachedResources();

    g_signal_handlers_disconnect_by_func(gtk_settings_get_default(),
                                         FuncToGpointer(theme_changed_cb),
                                         this);

    nsIRollupListener* rollupListener = nsBaseWidget::GetActiveRollupListener();
    if (rollupListener) {
        nsCOMPtr<nsIWidget> rollupWidget = rollupListener->GetRollupWidget();
        if (static_cast<nsIWidget *>(this) == rollupWidget) {
            rollupListener->Rollup(0, false, nullptr, nullptr);
        }
    }

    
    nsDragService *dragService = nsDragService::GetInstance();
    if (dragService && this == dragService->GetMostRecentDestWindow()) {
        dragService->ScheduleLeaveEvent();
    }

    NativeShow(false);

    if (mIMModule) {
        mIMModule->OnDestroyWindow(this);
    }

    
    if (gFocusWindow == this) {
        LOGFOCUS(("automatically losing focus...\n"));
        gFocusWindow = nullptr;
    }

#if (MOZ_WIDGET_GTK == 2) && defined(MOZ_X11)
    
    if (gPluginFocusWindow == this) {
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }
#endif 
  
    
    
    mThebesSurface = nullptr;

    GtkWidget *owningWidget = GetMozContainerWidget();
    if (mShell) {
        gtk_widget_destroy(mShell);
        mShell = nullptr;
        mContainer = nullptr;
        MOZ_ASSERT(!mGdkWindow,
                   "mGdkWindow should be NULL when mContainer is destroyed");
    }
    else if (mContainer) {
        gtk_widget_destroy(GTK_WIDGET(mContainer));
        mContainer = nullptr;
        MOZ_ASSERT(!mGdkWindow,
                   "mGdkWindow should be NULL when mContainer is destroyed");
    }
    else if (mGdkWindow) {
        
        
        
        
        DestroyChildWindows();

        gdk_window_set_user_data(mGdkWindow, nullptr);
        g_object_set_data(G_OBJECT(mGdkWindow), "nsWindow", nullptr);
        gdk_window_destroy(mGdkWindow);
        mGdkWindow = nullptr;
    }

    if (gInvisibleContainer && owningWidget == gInvisibleContainer) {
        CheckDestroyInvisibleContainer();
    }

#ifdef ACCESSIBILITY
     if (mRootAccessible) {
         mRootAccessible = nullptr;
     }
#endif

    
    OnDestroy();

    return NS_OK;
}

nsIWidget *
nsWindow::GetParent(void)
{
    return mParent;
}

float
nsWindow::GetDPI()
{
    GdkScreen *screen = gdk_display_get_default_screen(gdk_display_get_default());
    double heightInches = gdk_screen_get_height_mm(screen)/MM_PER_INCH_FLOAT;
    if (heightInches < 0.25) {
        
        return 96.0f;
    }
    return float(gdk_screen_get_height(screen)/heightInches);
}

double
nsWindow::GetDefaultScaleInternal()
{
#if (MOZ_WIDGET_GTK == 3)
    return GdkScaleFactor();
#else
    return gfxPlatformGtk::GetDPIScale();
#endif
}

NS_IMETHODIMP
nsWindow::SetParent(nsIWidget *aNewParent)
{
    if (mContainer || !mGdkWindow) {
        NS_NOTREACHED("nsWindow::SetParent called illegally");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;
    if (mParent) {
        mParent->RemoveChild(this);
    }

    mParent = aNewParent;

    GtkWidget* oldContainer = GetMozContainerWidget();
    if (!oldContainer) {
        
        
        MOZ_ASSERT(gdk_window_is_destroyed(mGdkWindow),
                   "live GdkWindow with no widget");
        return NS_OK;
    }

    if (aNewParent) {
        aNewParent->AddChild(this);
        ReparentNativeWidget(aNewParent);
    } else {
        
        
        
        
        GtkWidget* newContainer = EnsureInvisibleContainer();
        GdkWindow* newParentWindow = gtk_widget_get_window(newContainer);
        ReparentNativeWidgetInternal(aNewParent, newContainer, newParentWindow,
                                     oldContainer);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ReparentNativeWidget(nsIWidget* aNewParent)
{
    NS_PRECONDITION(aNewParent, "");
    NS_ASSERTION(!mIsDestroyed, "");
    NS_ASSERTION(!static_cast<nsWindow*>(aNewParent)->mIsDestroyed, "");

    GtkWidget* oldContainer = GetMozContainerWidget();
    if (!oldContainer) {
        
        
        MOZ_ASSERT(gdk_window_is_destroyed(mGdkWindow),
                   "live GdkWindow with no widget");
        return NS_OK;
    }
    MOZ_ASSERT(!gdk_window_is_destroyed(mGdkWindow),
               "destroyed GdkWindow with widget");
    
    nsWindow* newParent = static_cast<nsWindow*>(aNewParent);
    GdkWindow* newParentWindow = newParent->mGdkWindow;
    GtkWidget* newContainer = newParent->GetMozContainerWidget();
    GtkWindow* shell = GTK_WINDOW(mShell);

    if (shell && gtk_window_get_transient_for(shell)) {
      GtkWindow* topLevelParent =
          GTK_WINDOW(gtk_widget_get_toplevel(newContainer));
      gtk_window_set_transient_for(shell, topLevelParent);
    }

    ReparentNativeWidgetInternal(aNewParent, newContainer, newParentWindow,
                                 oldContainer);
    return NS_OK;
}

void
nsWindow::ReparentNativeWidgetInternal(nsIWidget* aNewParent,
                                       GtkWidget* aNewContainer,
                                       GdkWindow* aNewParentWindow,
                                       GtkWidget* aOldContainer)
{
    if (!aNewContainer) {
        
        MOZ_ASSERT(!aNewParentWindow ||
                   gdk_window_is_destroyed(aNewParentWindow),
                   "live GdkWindow with no widget");
        Destroy();
    } else {
        if (aNewContainer != aOldContainer) {
            MOZ_ASSERT(!gdk_window_is_destroyed(aNewParentWindow),
                       "destroyed GdkWindow with widget");
            SetWidgetForHierarchy(mGdkWindow, aOldContainer, aNewContainer);

            if (aOldContainer == gInvisibleContainer) {
                CheckDestroyInvisibleContainer();
            }
        }

        if (!mIsTopLevel) {
            gdk_window_reparent(mGdkWindow, aNewParentWindow,
                                DevicePixelsToGdkCoordRoundDown(mBounds.x),
                                DevicePixelsToGdkCoordRoundDown(mBounds.y));
        }
    }

    nsWindow* newParent = static_cast<nsWindow*>(aNewParent);
    bool parentHasMappedToplevel =
        newParent && newParent->mHasMappedToplevel;
    if (mHasMappedToplevel != parentHasMappedToplevel) {
        SetHasMappedToplevel(parentHasMappedToplevel);
    }
}

NS_IMETHODIMP
nsWindow::SetModal(bool aModal)
{
    LOG(("nsWindow::SetModal [%p] %d\n", (void *)this, aModal));
    if (mIsDestroyed)
        return aModal ? NS_ERROR_NOT_AVAILABLE : NS_OK;
    if (!mIsTopLevel || !mShell)
        return NS_ERROR_FAILURE;
    gtk_window_set_modal(GTK_WINDOW(mShell), aModal ? TRUE : FALSE);
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
    if (!mIsTopLevel || !mShell)  
      return NS_OK;

    double dpiScale = GetDefaultScale().scale;

    
    int32_t logWidth = std::max(NSToIntRound(mBounds.width / dpiScale), 1);
    int32_t logHeight = std::max(NSToIntRound(mBounds.height / dpiScale), 1);  

    

    nsCOMPtr<nsIScreen> screen;
    nsCOMPtr<nsIScreenManager> screenmgr = do_GetService("@mozilla.org/gfx/screenmanager;1");
    if (screenmgr) {
      screenmgr->ScreenForRect(*aX, *aY, logWidth, logHeight,
                               getter_AddRefs(screen));
    }

    
    if (!screen)
      return NS_OK;

    nsIntRect screenRect;
    if (mSizeMode != nsSizeMode_Fullscreen) {
      
      screen->GetAvailRectDisplayPix(&screenRect.x, &screenRect.y,
                                     &screenRect.width, &screenRect.height);
    } else {
      
      screen->GetRectDisplayPix(&screenRect.x, &screenRect.y,
                                &screenRect.width, &screenRect.height);
    }

    if (aAllowSlop) {
      if (*aX < screenRect.x - logWidth + kWindowPositionSlop)
          *aX = screenRect.x - logWidth + kWindowPositionSlop;
      else if (*aX >= screenRect.XMost() - kWindowPositionSlop)
          *aX = screenRect.XMost() - kWindowPositionSlop;

      if (*aY < screenRect.y - logHeight + kWindowPositionSlop)
          *aY = screenRect.y - logHeight + kWindowPositionSlop;
      else if (*aY >= screenRect.YMost() - kWindowPositionSlop)
          *aY = screenRect.YMost() - kWindowPositionSlop;
    } else {  
      if (*aX < screenRect.x)
          *aX = screenRect.x;
      else if (*aX >= screenRect.XMost() - logWidth)
          *aX = screenRect.XMost() - logWidth;

      if (*aY < screenRect.y)
          *aY = screenRect.y;
      else if (*aY >= screenRect.YMost() - logHeight)
          *aY = screenRect.YMost() - logHeight;
    }

    return NS_OK;
}

void nsWindow::SetSizeConstraints(const SizeConstraints& aConstraints)
{
    mSizeConstraints.mMinSize = GetSafeWindowSize(aConstraints.mMinSize);
    mSizeConstraints.mMaxSize = GetSafeWindowSize(aConstraints.mMaxSize);

    if (mShell) {
        GdkGeometry geometry;
        geometry.min_width = DevicePixelsToGdkCoordRoundUp(
                             mSizeConstraints.mMinSize.width);
        geometry.min_height = DevicePixelsToGdkCoordRoundUp(
                              mSizeConstraints.mMinSize.height);
        geometry.max_width = DevicePixelsToGdkCoordRoundDown(
                             mSizeConstraints.mMaxSize.width);
        geometry.max_height = DevicePixelsToGdkCoordRoundDown(
                              mSizeConstraints.mMaxSize.height);

        uint32_t hints = GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE;
        gtk_window_set_geometry_hints(GTK_WINDOW(mShell), nullptr,
                                      &geometry, GdkWindowHints(hints));
    }
}

NS_IMETHODIMP
nsWindow::Show(bool aState)
{
    if (aState == mIsShown)
        return NS_OK;

    
    if (mIsShown && !aState) {
        ClearCachedResources();
    }

    mIsShown = aState;

    LOG(("nsWindow::Show [%p] state %d\n", (void *)this, aState));

    if (aState) {
        
        
        SetHasMappedToplevel(mHasMappedToplevel);
    }

    
    
    
    if ((aState && !AreBoundsSane()) || !mCreated) {
        LOG(("\tbounds are insane or window hasn't been created yet\n"));
        mNeedsShow = true;
        return NS_OK;
    }

    
    if (!aState)
        mNeedsShow = false;

    
    
    if (aState) {
        if (mNeedsMove) {
            NativeResize(mBounds.x, mBounds.y, mBounds.width, mBounds.height,
                         false);
        } else if (mNeedsResize) {
            NativeResize(mBounds.width, mBounds.height, false);
        }
    }

#ifdef ACCESSIBILITY
    if (aState && a11y::ShouldA11yBeEnabled())
        CreateRootAccessible();
#endif

    NativeShow(aState);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aWidth, double aHeight, bool aRepaint)
{
    CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels() ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
    int32_t width = NSToIntRound(scale.scale * aWidth);
    int32_t height = NSToIntRound(scale.scale * aHeight);
    ConstrainSize(&width, &height);

    
    
    

    mBounds.SizeTo(width, height);

    if (!mCreated)
        return NS_OK;

    
    
    

    
    if (mIsShown) {
        
        if (AreBoundsSane()) {
            
            

            
            
            
            
            if (mNeedsMove)
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
    
    
    else {
        if (AreBoundsSane() && mListenForResizes) {
            
            
            
            NativeResize(width, height, aRepaint);
        }
        else {
            mNeedsResize = true;
        }
    }

    NotifyRollupGeometryChange();
    ResizePluginSocketWidget();

    
    if (mIsTopLevel || mListenForResizes) {
        DispatchResized(width, height);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(double aX, double aY, double aWidth, double aHeight,
                 bool aRepaint)
{
    CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels() ? GetDefaultScale()
                                    : CSSToLayoutDeviceScale(1.0);
    int32_t width = NSToIntRound(scale.scale * aWidth);
    int32_t height = NSToIntRound(scale.scale * aHeight);
    ConstrainSize(&width, &height);

    int32_t x = NSToIntRound(scale.scale * aX);
    int32_t y = NSToIntRound(scale.scale * aY);
    mBounds.x = x;
    mBounds.y = y;
    mBounds.SizeTo(width, height);

    mNeedsMove = true;

    if (!mCreated)
        return NS_OK;

    
    
    

    
    if (mIsShown) {
        
        if (AreBoundsSane()) {
            
            NativeResize(x, y, width, height, aRepaint);
            
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
    
    
    else {
        if (AreBoundsSane() && mListenForResizes){
            
            
            
            NativeResize(x, y, width, height, aRepaint);
        }
        else {
            mNeedsResize = true;
        }
    }

    NotifyRollupGeometryChange();
    ResizePluginSocketWidget();

    if (mIsTopLevel || mListenForResizes) {
        DispatchResized(width, height);
    }

    return NS_OK;
}

void
nsWindow::ResizePluginSocketWidget()
{
    
    
    
    if (mWindowType == eWindowType_plugin_ipc_chrome) {
        nsPluginNativeWindowGtk* wrapper = (nsPluginNativeWindowGtk*)
          GetNativeData(NS_NATIVE_PLUGIN_OBJECT_PTR);
        if (wrapper) {
            wrapper->width = mBounds.width;
            wrapper->height = mBounds.height;
            wrapper->SetAllocation();
        }
    }
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



NS_IMETHODIMP
nsWindow::Move(double aX, double aY)
{
    LOG(("nsWindow::Move [%p] %f %f\n", (void *)this,
         aX, aY));

    CSSToLayoutDeviceScale scale = BoundsUseDisplayPixels() ? GetDefaultScale()
                                   : CSSToLayoutDeviceScale(1.0);
    int32_t x = NSToIntRound(aX * scale.scale);
    int32_t y = NSToIntRound(aY * scale.scale);

    if (mWindowType == eWindowType_toplevel ||
        mWindowType == eWindowType_dialog) {
        SetSizeMode(nsSizeMode_Normal);
    }

    
    
    
    if (x == mBounds.x && y == mBounds.y &&
        mWindowType != eWindowType_popup)
        return NS_OK;

    

    mBounds.x = x;
    mBounds.y = y;

    if (!mCreated)
        return NS_OK;

    mNeedsMove = false;

    GdkPoint point = DevicePixelsToGdkPointRoundDown(nsIntPoint(x, y));

    if (mIsTopLevel) {
        gtk_window_move(GTK_WINDOW(mShell), point.x, point.y);
    }
    else if (mGdkWindow) {
        gdk_window_move(mGdkWindow, point.x, point.y);
    }

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

void
nsWindow::SetZIndex(int32_t aZIndex)
{
    nsIWidget* oldPrev = GetPrevSibling();

    nsBaseWidget::SetZIndex(aZIndex);

    if (GetPrevSibling() == oldPrev) {
        return;
    }

    NS_ASSERTION(!mContainer, "Expected Mozilla child widget");

    
    

    if (!GetNextSibling()) {
        
        if (mGdkWindow)
            gdk_window_raise(mGdkWindow);
    } else {
        
        for (nsWindow* w = this; w;
             w = static_cast<nsWindow*>(w->GetPrevSibling())) {
            if (w->mGdkWindow)
                gdk_window_lower(w->mGdkWindow);
        }
    }
}

NS_IMETHODIMP
nsWindow::SetSizeMode(int32_t aMode)
{
    nsresult rv;

    LOG(("nsWindow::SetSizeMode [%p] %d\n", (void *)this, aMode));

    
    rv = nsBaseWidget::SetSizeMode(aMode);

    
    
    if (!mShell || mSizeState == mSizeMode) {
        return rv;
    }

    switch (aMode) {
    case nsSizeMode_Maximized:
        gtk_window_maximize(GTK_WINDOW(mShell));
        break;
    case nsSizeMode_Minimized:
        gtk_window_iconify(GTK_WINDOW(mShell));
        break;
    case nsSizeMode_Fullscreen:
        MakeFullScreen(true);
        break;

    default:
        
        if (mSizeState == nsSizeMode_Minimized)
            gtk_window_deiconify(GTK_WINDOW(mShell));
        else if (mSizeState == nsSizeMode_Maximized)
            gtk_window_unmaximize(GTK_WINDOW(mShell));
        break;
    }

    mSizeState = mSizeMode;

    return rv;
}

typedef void (* SetUserTimeFunc)(GdkWindow* aWindow, guint32 aTimestamp);



static void
SetUserTimeAndStartupIDForActivatedWindow(GtkWidget* aWindow)
{
    nsGTKToolkit* GTKToolkit = nsGTKToolkit::GetToolkit();
    if (!GTKToolkit)
        return;

    nsAutoCString desktopStartupID;
    GTKToolkit->GetDesktopStartupID(&desktopStartupID);
    if (desktopStartupID.IsEmpty()) {
        
        
        
        
        uint32_t timestamp = GTKToolkit->GetFocusTimestamp();
        if (timestamp) {
            gdk_window_focus(gtk_widget_get_window(aWindow), timestamp);
            GTKToolkit->SetFocusTimestamp(0);
        }
        return;
    }

#if defined(MOZ_ENABLE_STARTUP_NOTIFICATION)
    GdkWindow* gdkWindow = gtk_widget_get_window(aWindow);
  
    GdkScreen* screen = gdk_window_get_screen(gdkWindow);
    SnDisplay* snd =
        sn_display_new(gdk_x11_display_get_xdisplay(gdk_window_get_display(gdkWindow)), 
                       nullptr, nullptr);
    if (!snd)
        return;
    SnLauncheeContext* ctx =
        sn_launchee_context_new(snd, gdk_screen_get_number(screen),
                                desktopStartupID.get());
    if (!ctx) {
        sn_display_unref(snd);
        return;
    }

    if (sn_launchee_context_get_id_has_timestamp(ctx)) {
        gdk_x11_window_set_user_time(gdkWindow, sn_launchee_context_get_timestamp(ctx));
    }

    sn_launchee_context_setup_window(ctx, gdk_x11_window_get_xid(gdkWindow));
    sn_launchee_context_complete(ctx);

    sn_launchee_context_unref(ctx);
    sn_display_unref(snd);
#endif

    
    
    GTKToolkit->SetFocusTimestamp(0);
    GTKToolkit->SetDesktopStartupID(EmptyCString());
}

 guint32
nsWindow::GetLastUserInputTime()
{
    
    
    
    
    
    guint32 timestamp =
            gdk_x11_display_get_user_time(gdk_display_get_default());
    if (sLastUserInputTime != GDK_CURRENT_TIME &&
        TimestampIsNewerThan(sLastUserInputTime, timestamp)) {
        return sLastUserInputTime;
    }       

    return timestamp;
}

NS_IMETHODIMP
nsWindow::SetFocus(bool aRaise)
{
    
    

    LOGFOCUS(("  SetFocus %d [%p]\n", aRaise, (void *)this));

    GtkWidget *owningWidget = GetMozContainerWidget();
    if (!owningWidget)
        return NS_ERROR_FAILURE;

    
    
    GtkWidget *toplevelWidget = gtk_widget_get_toplevel(owningWidget);

    if (gRaiseWindows && aRaise && toplevelWidget &&
        !gtk_widget_has_focus(owningWidget) &&
        !gtk_widget_has_focus(toplevelWidget)) {
        GtkWidget* top_window = GetToplevelWidget();
        if (top_window && (gtk_widget_get_visible(top_window)))
        {
            gdk_window_show_unraised(gtk_widget_get_window(top_window));
            
            SetUrgencyHint(top_window, false);
        }
    }

    nsRefPtr<nsWindow> owningWindow = get_window_for_gtk_widget(owningWidget);
    if (!owningWindow)
        return NS_ERROR_FAILURE;

    if (aRaise) {
        

        
        
        
        if (gRaiseWindows && owningWindow->mIsShown && owningWindow->mShell &&
            !gtk_window_is_active(GTK_WINDOW(owningWindow->mShell))) {

            uint32_t timestamp = GDK_CURRENT_TIME;

            nsGTKToolkit* GTKToolkit = nsGTKToolkit::GetToolkit();
            if (GTKToolkit)
                timestamp = GTKToolkit->GetFocusTimestamp();

            LOGFOCUS(("  requesting toplevel activation [%p]\n", (void *)this));
            NS_ASSERTION(owningWindow->mWindowType != eWindowType_popup
                         || mParent,
                         "Presenting an override-redirect window");
            gtk_window_present_with_time(GTK_WINDOW(owningWindow->mShell), timestamp);

            if (GTKToolkit)
                GTKToolkit->SetFocusTimestamp(0);
        }

        return NS_OK;
    }

    
    

    
    
    
    
    if (!gtk_widget_is_focus(owningWidget)) {
        
        
        
        
        gBlockActivateEvent = true;
        gtk_widget_grab_focus(owningWidget);
        gBlockActivateEvent = false;
    }

    
    if (gFocusWindow == this) {
        LOGFOCUS(("  already have focus [%p]\n", (void *)this));
        return NS_OK;
    }

    
    gFocusWindow = this;

    if (mIMModule) {
        mIMModule->OnFocusWindow(this);
    }

    LOGFOCUS(("  widget now has focus in SetFocus() [%p]\n",
              (void *)this));

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetScreenBounds(nsIntRect &aRect)
{
    if (mIsTopLevel && mContainer) {
        
        gint x, y;
        gdk_window_get_root_origin(gtk_widget_get_window(GTK_WIDGET(mContainer)), &x, &y);
        aRect.MoveTo(LayoutDevicePixel::ToUntyped(GdkPointToDevicePixels({ x, y })));
    }
    else {
        aRect.MoveTo(WidgetToScreenOffsetUntyped());
    }
    
    
    
    
    aRect.SizeTo(mBounds.Size());
    LOG(("GetScreenBounds %d,%d | %dx%d\n",
         aRect.x, aRect.y, aRect.width, aRect.height));
    return NS_OK;
}

gfx::IntSize
nsWindow::GetClientSize()
{
  return gfx::IntSize(mBounds.width, mBounds.height);
}

NS_IMETHODIMP
nsWindow::GetClientBounds(nsIntRect &aRect)
{
    
    
    
    GetBounds(aRect);
    aRect.MoveBy(GetClientOffset());

    return NS_OK;
}

nsIntPoint
nsWindow::GetClientOffset()
{
    PROFILER_LABEL("nsWindow", "GetClientOffset", js::ProfileEntry::Category::GRAPHICS);

    if (!mIsTopLevel) {
        return nsIntPoint(0, 0);
    }

    GdkAtom cardinal_atom = gdk_x11_xatom_to_atom(XA_CARDINAL);

    GdkAtom type_returned;
    int format_returned;
    int length_returned;
    long *frame_extents;
    GdkWindow* window;

    if (!mShell || !(window = gtk_widget_get_window(mShell)) ||
        !gdk_property_get(window,
                          gdk_atom_intern ("_NET_FRAME_EXTENTS", FALSE),
                          cardinal_atom,
                          0, 
                          4*4, 
                          FALSE, 
                          &type_returned,
                          &format_returned,
                          &length_returned,
                          (guchar **) &frame_extents) ||
        length_returned/sizeof(glong) != 4) {

        return nsIntPoint(0, 0);
    }

    
    int32_t left = int32_t(frame_extents[0]);
    int32_t top = int32_t(frame_extents[2]);

    g_free(frame_extents);

    return nsIntPoint(left, top);
}

NS_IMETHODIMP
nsWindow::SetCursor(nsCursor aCursor)
{
    
    
    if (!mContainer && mGdkWindow) {
        nsWindow *window = GetContainerWindow();
        if (!window)
            return NS_ERROR_FAILURE;

        return window->SetCursor(aCursor);
    }

    
    if (aCursor != mCursor || mUpdateCursor) {
        GdkCursor *newCursor = nullptr;
        mUpdateCursor = false;

        newCursor = get_gtk_cursor(aCursor);

        if (nullptr != newCursor) {
            mCursor = aCursor;

            if (!mContainer)
                return NS_OK;

            gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(mContainer)), newCursor);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetCursor(imgIContainer* aCursor,
                    uint32_t aHotspotX, uint32_t aHotspotY)
{
    
    
    if (!mContainer && mGdkWindow) {
        nsWindow *window = GetContainerWindow();
        if (!window)
            return NS_ERROR_FAILURE;

        return window->SetCursor(aCursor, aHotspotX, aHotspotY);
    }

    mCursor = nsCursor(-1);

    
    GdkPixbuf* pixbuf = nsImageToPixbuf::ImageToPixbuf(aCursor);
    if (!pixbuf)
        return NS_ERROR_NOT_AVAILABLE;

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    
    
    
    
    if (width > 128 || height > 128) {
        g_object_unref(pixbuf);
        return NS_ERROR_NOT_AVAILABLE;
    }

    
    
    
    if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
        GdkPixbuf* alphaBuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
        g_object_unref(pixbuf);
        if (!alphaBuf) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        pixbuf = alphaBuf;
    }

    GdkCursor* cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(),
                                                   pixbuf,
                                                   aHotspotX, aHotspotY);
    g_object_unref(pixbuf);
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    if (cursor) {
        if (mContainer) {
            gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(mContainer)), cursor);
            rv = NS_OK;
        }
#if (MOZ_WIDGET_GTK == 3)
        g_object_unref(cursor);
#else
        gdk_cursor_unref(cursor);
#endif
    }

    return rv;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsIntRect &aRect)
{
    if (!mGdkWindow)
        return NS_OK;

    GdkRectangle rect = DevicePixelsToGdkRectRoundOut(aRect);
    gdk_window_invalidate_rect(mGdkWindow, &rect, FALSE);

    LOGDRAW(("Invalidate (rect) [%p]: %d %d %d %d\n", (void *)this,
             rect.x, rect.y, rect.width, rect.height));

    return NS_OK;
}

void*
nsWindow::GetNativeData(uint32_t aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_WIDGET: {
        if (!mGdkWindow)
            return nullptr;

        return mGdkWindow;
        break;
    }

    case NS_NATIVE_PLUGIN_PORT:
        return SetupPluginPort();
        break;

    case NS_NATIVE_PLUGIN_ID:
        if (!mPluginNativeWindow) {
          NS_WARNING("no native plugin instance!");
          return nullptr;
        }
        
        return (void*)mPluginNativeWindow->window;
        break;

    case NS_NATIVE_DISPLAY:
#ifdef MOZ_X11
        return GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
#else
        return nullptr;
#endif 
        break;

    case NS_NATIVE_SHELLWIDGET:
        return GetToplevelWidget();

    case NS_NATIVE_SHAREABLE_WINDOW:
        return (void *) GDK_WINDOW_XID(gdk_window_get_toplevel(mGdkWindow));
    case NS_NATIVE_PLUGIN_OBJECT_PTR:
        return (void *) mPluginNativeWindow;
    default:
        NS_WARNING("nsWindow::GetNativeData called with bad value");
        return nullptr;
    }
}

void
nsWindow::SetNativeData(uint32_t aDataType, uintptr_t aVal)
{
    if (aDataType != NS_NATIVE_PLUGIN_OBJECT_PTR) {
        NS_WARNING("nsWindow::SetNativeData called with bad value");
        return;
    }
    mPluginNativeWindow = (nsPluginNativeWindowGtk*)aVal;
}

NS_IMETHODIMP
nsWindow::SetTitle(const nsAString& aTitle)
{
    if (!mShell)
        return NS_OK;

    
#define UTF8_FOLLOWBYTE(ch) (((ch) & 0xC0) == 0x80)
    NS_ConvertUTF16toUTF8 titleUTF8(aTitle);
    if (titleUTF8.Length() > NS_WINDOW_TITLE_MAX_LENGTH) {
        
        
        uint32_t len = NS_WINDOW_TITLE_MAX_LENGTH;
        while(UTF8_FOLLOWBYTE(titleUTF8[len]))
            --len;
        titleUTF8.Truncate(len);
    }
    gtk_window_set_title(GTK_WINDOW(mShell), (const char *)titleUTF8.get());

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIcon(const nsAString& aIconSpec)
{
    if (!mShell)
        return NS_OK;

    nsAutoCString iconName;
    
    if (aIconSpec.EqualsLiteral("default")) {
        nsXPIDLString brandName;
        GetBrandName(brandName);
        AppendUTF16toUTF8(brandName, iconName);
        ToLowerCase(iconName);
    } else {
        AppendUTF16toUTF8(aIconSpec, iconName);
    }
    
    nsCOMPtr<nsIFile> iconFile;
    nsAutoCString path;

    gint *iconSizes =
        gtk_icon_theme_get_icon_sizes(gtk_icon_theme_get_default(),
                                      iconName.get());
    bool foundIcon = (iconSizes[0] != 0);
    g_free(iconSizes);

    if (!foundIcon) {
        
        
        

        const char extensions[6][7] = { ".png", "16.png", "32.png", "48.png",
                                    ".xpm", "16.xpm" };

        for (uint32_t i = 0; i < ArrayLength(extensions); i++) {
            
            if (i == ArrayLength(extensions) - 2 && foundIcon)
                break;

            nsAutoString extension;
            extension.AppendASCII(extensions[i]);

            ResolveIconName(aIconSpec, extension, getter_AddRefs(iconFile));
            if (iconFile) {
                iconFile->GetNativePath(path);
                GdkPixbuf *icon = gdk_pixbuf_new_from_file(path.get(), nullptr);
                if (icon) {
                    gtk_icon_theme_add_builtin_icon(iconName.get(),
                                                    gdk_pixbuf_get_height(icon),
                                                    icon);
                    g_object_unref(icon);
                    foundIcon = true;
                }
            }
        }
    }

    
    if (foundIcon) {
        gtk_window_set_icon_name(GTK_WINDOW(mShell), iconName.get());
    }

    return NS_OK;
}


LayoutDeviceIntPoint
nsWindow::WidgetToScreenOffset()
{
    gint x = 0, y = 0;

    if (mGdkWindow) {
        gdk_window_get_origin(mGdkWindow, &x, &y);
    }

    return GdkPointToDevicePixels({ x, y });
}

NS_IMETHODIMP
nsWindow::EnableDragDrop(bool aEnable)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureMouse(bool aCapture)
{
    LOG(("CaptureMouse %p\n", (void *)this));

    if (!mGdkWindow)
        return NS_OK;

    if (!mShell)
        return NS_ERROR_FAILURE;

    if (aCapture) {
        gtk_grab_add(mShell);
        GrabPointer(GetLastUserInputTime());
    }
    else {
        ReleaseGrabs();
        gtk_grab_remove(mShell);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureRollupEvents(nsIRollupListener *aListener,
                              bool               aDoCapture)
{
    if (!mGdkWindow)
        return NS_OK;

    if (!mShell)
        return NS_ERROR_FAILURE;

    LOG(("CaptureRollupEvents %p %i\n", this, int(aDoCapture)));

    if (aDoCapture) {
        gRollupListener = aListener;
        
        if (!nsWindow::DragInProgress()) {
            
            
            
            
            
            gtk_grab_add(mShell);
            GrabPointer(GetLastUserInputTime());
        }
    }
    else {
        if (!nsWindow::DragInProgress()) {
            ReleaseGrabs();
        }
        
        
        
        gtk_grab_remove(mShell);
        gRollupListener = nullptr;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetAttention(int32_t aCycleCount)
{
    LOG(("nsWindow::GetAttention [%p]\n", (void *)this));

    GtkWidget* top_window = GetToplevelWidget();
    GtkWidget* top_focused_window =
        gFocusWindow ? gFocusWindow->GetToplevelWidget() : nullptr;

    
    if (top_window && (gtk_widget_get_visible(top_window)) &&
        top_window != top_focused_window) {
        SetUrgencyHint(top_window, true);
    }

    return NS_OK;
}

bool
nsWindow::HasPendingInputEvent()
{
    
    
    
    
    
    bool haveEvent;
#ifdef MOZ_X11
    XEvent ev;
    GdkDisplay* gdkDisplay = gdk_display_get_default();
    if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
        Display *display = GDK_DISPLAY_XDISPLAY(gdkDisplay);
        haveEvent =
            XCheckMaskEvent(display,
                            KeyPressMask | KeyReleaseMask | ButtonPressMask |
                            ButtonReleaseMask | EnterWindowMask | LeaveWindowMask |
                            PointerMotionMask | PointerMotionHintMask |
                            Button1MotionMask | Button2MotionMask |
                            Button3MotionMask | Button4MotionMask |
                            Button5MotionMask | ButtonMotionMask | KeymapStateMask |
                            VisibilityChangeMask | StructureNotifyMask |
                            ResizeRedirectMask | SubstructureNotifyMask |
                            SubstructureRedirectMask | FocusChangeMask |
                            PropertyChangeMask | ColormapChangeMask |
                            OwnerGrabButtonMask, &ev);
        if (haveEvent) {
            XPutBackEvent(display, &ev);
        }
    }
#else
    haveEvent = false;
#endif
    return haveEvent;
}

#if 0
#ifdef DEBUG


#define CAPS_LOCK_IS_ON \
(KeymapWrapper::AreModifiersCurrentlyActive(KeymapWrapper::CAPS_LOCK))

#define WANT_PAINT_FLASHING \
(debug_WantPaintFlashing() && CAPS_LOCK_IS_ON)

#ifdef MOZ_X11
static void
gdk_window_flash(GdkWindow *    aGdkWindow,
                 unsigned int   aTimes,
                 unsigned int   aInterval,  
                 GdkRegion *    aRegion)
{
  gint         x;
  gint         y;
  gint         width;
  gint         height;
  guint        i;
  GdkGC *      gc = 0;
  GdkColor     white;

#if (MOZ_WIDGET_GTK == 2)
  gdk_window_get_geometry(aGdkWindow,nullptr,nullptr,&width,&height,nullptr);
#else
  gdk_window_get_geometry(aGdkWindow,nullptr,nullptr,&width,&height);
#endif

  gdk_window_get_origin (aGdkWindow,
                         &x,
                         &y);

  gc = gdk_gc_new(gdk_get_default_root_window());

  white.pixel = WhitePixel(gdk_display,DefaultScreen(gdk_display));

  gdk_gc_set_foreground(gc,&white);
  gdk_gc_set_function(gc,GDK_XOR);
  gdk_gc_set_subwindow(gc,GDK_INCLUDE_INFERIORS);

  gdk_region_offset(aRegion, x, y);
  gdk_gc_set_clip_region(gc, aRegion);

  



  for (i = 0; i < aTimes * 2; i++)
  {
    gdk_draw_rectangle(gdk_get_default_root_window(),
                       gc,
                       TRUE,
                       x,
                       y,
                       width,
                       height);

    gdk_flush();

    PR_Sleep(PR_MillisecondsToInterval(aInterval));
  }

  gdk_gc_destroy(gc);

  gdk_region_offset(aRegion, -x, -y);
}
#endif 
#endif 
#endif

struct ExposeRegion
{
    nsIntRegion mRegion;

#if (MOZ_WIDGET_GTK == 2)
    GdkRectangle *mRects;
    GdkRectangle *mRectsEnd;

    ExposeRegion() : mRects(nullptr)
    {
    }
    ~ExposeRegion()
    {
        g_free(mRects);
    }
    bool Init(GdkEventExpose *aEvent)
    {
        gint nrects;
        gdk_region_get_rectangles(aEvent->region, &mRects, &nrects);

        if (nrects > MAX_RECTS_IN_REGION) {
            
            mRects[0] = aEvent->area;
            nrects = 1;
        }

        mRectsEnd = mRects + nrects;

        for (GdkRectangle *r = mRects; r < mRectsEnd; r++) {
            mRegion.Or(mRegion, nsIntRect(r->x, r->y, r->width, r->height));
            LOGDRAW(("\t%d %d %d %d\n", r->x, r->y, r->width, r->height));
        }
        return true;
    }

#else
# ifdef cairo_copy_clip_rectangle_list
#  error "Looks like we're including Mozilla's cairo instead of system cairo"
# endif
    cairo_rectangle_list_t *mRects;

    ExposeRegion() : mRects(nullptr)
    {
    }
    ~ExposeRegion()
    {
        cairo_rectangle_list_destroy(mRects);
    }
    bool Init(cairo_t* cr)
    {
        mRects = cairo_copy_clip_rectangle_list(cr);
        if (mRects->status != CAIRO_STATUS_SUCCESS) {
            NS_WARNING("Failed to obtain cairo rectangle list.");
            return false;
        }

        for (int i = 0; i < mRects->num_rectangles; i++)  {
            const cairo_rectangle_t& r = mRects->rectangles[i];
            mRegion.Or(mRegion, nsIntRect(r.x, r.y, r.width, r.height));
            LOGDRAW(("\t%d %d %d %d\n", r.x, r.y, r.width, r.height));
        }
        return true;
    }
#endif
};

#if (MOZ_WIDGET_GTK == 2)
gboolean
nsWindow::OnExposeEvent(GdkEventExpose *aEvent)
#else
gboolean
nsWindow::OnExposeEvent(cairo_t *cr)
#endif
{
    if (mIsDestroyed) {
        return FALSE;
    }

    
    if (!mGdkWindow || mIsFullyObscured || !mHasMappedToplevel)
        return FALSE;

    nsIWidgetListener *listener =
        mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
    if (!listener)
        return FALSE;

    ExposeRegion exposeRegion;
#if (MOZ_WIDGET_GTK == 2)
    if (!exposeRegion.Init(aEvent)) {
#else
    if (!exposeRegion.Init(cr)) {
#endif
        return FALSE;
    }

    gint scale = GdkScaleFactor();
    nsIntRegion& region = exposeRegion.mRegion;
    region.ScaleRoundOut(scale, scale);

    ClientLayerManager *clientLayers =
        (GetLayerManager()->GetBackendType() == LayersBackend::LAYERS_CLIENT)
        ? static_cast<ClientLayerManager*>(GetLayerManager())
        : nullptr;

    if (clientLayers && mCompositorParent) {
        
        
        clientLayers->SetNeedsComposite(true);
        clientLayers->SendInvalidRegion(region);
    }

    
    
    {
        listener->WillPaintWindow(this);

        
        
        if (!mGdkWindow)
            return TRUE;

        
        
        listener =
            mAttachedWidgetListener ? mAttachedWidgetListener : mWidgetListener;
        if (!listener)
            return FALSE;
    }

    if (clientLayers && mCompositorParent && clientLayers->NeedsComposite()) {
        mCompositorParent->ScheduleRenderOnCompositorThread();
        clientLayers->SetNeedsComposite(false);
    }

    LOGDRAW(("sending expose event [%p] %p 0x%lx (rects follow):\n",
             (void *)this, (void *)mGdkWindow,
             gdk_x11_window_get_xid(mGdkWindow)));

    
    
    
    region.And(region, nsIntRect(0, 0, mBounds.width, mBounds.height));

    bool shaped = false;
    if (eTransparencyTransparent == GetTransparencyMode()) {
        GdkScreen *screen = gdk_window_get_screen(mGdkWindow);
        if (gdk_screen_is_composited(screen) &&
            gdk_window_get_visual(mGdkWindow) ==
            gdk_screen_get_rgba_visual(screen)) {
            
            
            static_cast<nsWindow*>(GetTopLevelWidget())->
                ClearTransparencyBitmap();
        } else {
            shaped = true;
        }
    }

    if (!shaped) {
        GList *children =
            gdk_window_peek_children(mGdkWindow);
        while (children) {
            GdkWindow *gdkWin = GDK_WINDOW(children->data);
            nsWindow *kid = get_window_for_gdk_window(gdkWin);
            if (kid && gdk_window_is_visible(gdkWin)) {
                nsAutoTArray<nsIntRect,1> clipRects;
                kid->GetWindowClipRegion(&clipRects);
                nsIntRect bounds;
                kid->GetBounds(bounds);
                for (uint32_t i = 0; i < clipRects.Length(); ++i) {
                    nsIntRect r = clipRects[i] + bounds.TopLeft();
                    region.Sub(region, r);
                }
            }
            children = children->next;
        }
    }

    if (region.IsEmpty()) {
        return TRUE;
    }

    
    if (GetLayerManager()->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
        listener->PaintWindow(this, region);
        listener->DidPaintWindow();
        return TRUE;
    }

    gfxASurface* surf;
#if (MOZ_WIDGET_GTK == 2)
    surf = GetThebesSurface();
#else
    surf = GetThebesSurface(cr);
#endif

    nsRefPtr<gfxContext> ctx;
    if (gfxPlatform::GetPlatform()->
            SupportsAzureContentForType(BackendType::CAIRO)) {
        IntSize intSize(surf->GetSize().width, surf->GetSize().height);
        RefPtr<DrawTarget> dt =
          gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(surf, intSize);
        ctx = new gfxContext(dt);
    } else if (gfxPlatform::GetPlatform()->
                   SupportsAzureContentForType(BackendType::SKIA) &&
               surf->GetType() == gfxSurfaceType::Image) {
       gfxImageSurface* imgSurf = static_cast<gfxImageSurface*>(surf);
       SurfaceFormat format = ImageFormatToSurfaceFormat(imgSurf->Format());
       IntSize intSize(surf->GetSize().width, surf->GetSize().height);
       RefPtr<DrawTarget> dt =
         gfxPlatform::GetPlatform()->CreateDrawTargetForData(
                        imgSurf->Data(), intSize, imgSurf->Stride(), format);
       ctx = new gfxContext(dt);
    } else {
        MOZ_CRASH("Unexpected content type");
    }

#ifdef MOZ_X11
    nsIntRect boundsRect; 

    ctx->NewPath();
    if (shaped) {
        
        
        
        
        boundsRect = region.GetBounds();
        ctx->Rectangle(gfxRect(boundsRect.x, boundsRect.y,
                               boundsRect.width, boundsRect.height));
    } else {
        gfxUtils::PathFromRegion(ctx, region);
    }
    ctx->Clip();

    BufferMode layerBuffering;
    if (shaped) {
        
        
        
        layerBuffering = mozilla::layers::BufferMode::BUFFER_NONE;
        ctx->PushGroup(gfxContentType::COLOR_ALPHA);
#ifdef MOZ_HAVE_SHMIMAGE
    } else if (nsShmImage::UseShm()) {
        
        layerBuffering = mozilla::layers::BufferMode::BUFFER_NONE;
#endif
    } else {
        
        layerBuffering = mozilla::layers::BufferMode::BUFFERED;
    }

#if 0
    
    
    
#ifdef DEBUG
    
    if (0 && WANT_PAINT_FLASHING && gtk_widget_get_window(aEvent))
        gdk_window_flash(mGdkWindow, 1, 100, aEvent->region);
#endif
#endif

#endif 

    bool painted = false;
    {
      if (GetLayerManager()->GetBackendType() == LayersBackend::LAYERS_BASIC) {
        AutoLayerManagerSetup setupLayerManager(this, ctx, layerBuffering);
        painted = listener->PaintWindow(this, region);
      }
    }

#ifdef MOZ_X11
    
    
    if (shaped) {
        if (MOZ_LIKELY(!mIsDestroyed)) {
            if (painted) {
                nsRefPtr<gfxPattern> pattern = ctx->PopGroup();

                UpdateAlpha(pattern, boundsRect);

                ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
                ctx->SetPattern(pattern);
                ctx->Paint();
            }
        }
    }
#  ifdef MOZ_HAVE_SHMIMAGE
    if (mShmImage && MOZ_LIKELY(!mIsDestroyed)) {
#if (MOZ_WIDGET_GTK == 2)
        mShmImage->Put(mGdkWindow, exposeRegion.mRects, exposeRegion.mRectsEnd);
#else
        mShmImage->Put(mGdkWindow, exposeRegion.mRects);
#endif
    }
#  endif  
#endif 

    listener->DidPaintWindow();

    
#if (MOZ_WIDGET_GTK == 2)
    GdkRegion* dirtyArea = gdk_window_get_update_area(mGdkWindow);
#else
    cairo_region_t* dirtyArea = gdk_window_get_update_area(mGdkWindow);
#endif

    if (dirtyArea) {
        gdk_window_invalidate_region(mGdkWindow, dirtyArea, false);
#if (MOZ_WIDGET_GTK == 2)
        gdk_region_destroy(dirtyArea);
#else
        cairo_region_destroy(dirtyArea);
#endif
        gdk_window_process_updates(mGdkWindow, false);
    }

    
    return TRUE;
}

void
nsWindow::UpdateAlpha(gfxPattern* aPattern, nsIntRect aBoundsRect)
{
    
    
    int32_t stride = GetAlignedStride<4>(BytesPerPixel(SurfaceFormat::A8) *
                                         aBoundsRect.width);
    int32_t bufferSize = stride * aBoundsRect.height;
    nsAutoArrayPtr<uint8_t> imageBuffer(new (std::nothrow) uint8_t[bufferSize]);
    RefPtr<DrawTarget> drawTarget = gfxPlatform::GetPlatform()->
        CreateDrawTargetForData(imageBuffer, aBoundsRect.Size(),
                                stride, SurfaceFormat::A8);

    if (drawTarget) {
        Matrix transform = Matrix::Translation(-aBoundsRect.x, -aBoundsRect.y);
        drawTarget->SetTransform(transform);

        drawTarget->FillRect(Rect(aBoundsRect.x, aBoundsRect.y, aBoundsRect.width, aBoundsRect.height),
                             *aPattern->GetPattern(drawTarget),
                             DrawOptions(1.0, CompositionOp::OP_SOURCE));
    }
    UpdateTranslucentWindowAlphaInternal(aBoundsRect, imageBuffer, stride);
}

gboolean
nsWindow::OnConfigureEvent(GtkWidget *aWidget, GdkEventConfigure *aEvent)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    LOG(("configure event [%p] %d %d %d %d\n", (void *)this,
         aEvent->x, aEvent->y, aEvent->width, aEvent->height));

    nsIntRect screenBounds;
    GetScreenBounds(screenBounds);

    if (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog) {
        
        
        if (mBounds.x != screenBounds.x ||
            mBounds.y != screenBounds.y) {
            CheckForRollup(0, 0, false, true);
        }
    }

    
    

    
    
    NS_ASSERTION(GTK_IS_WINDOW(aWidget),
                 "Configure event on widget that is not a GtkWindow");
    gint type;
    g_object_get(aWidget, "type", &type, nullptr);
    if (type == GTK_WINDOW_POPUP) {
        
        
        
        
        
        
        
        
        
        
        
        
        return FALSE;
    }

    mBounds.MoveTo(screenBounds.TopLeft());

    
    
    NotifyWindowMoved(mBounds.x, mBounds.y);

    return FALSE;
}

void
nsWindow::OnContainerUnrealize()
{
    
    
    

    if (mGdkWindow) {
        DestroyChildWindows();

        g_object_set_data(G_OBJECT(mGdkWindow), "nsWindow", nullptr);
        mGdkWindow = nullptr;
    }
}

void
nsWindow::OnSizeAllocate(GtkAllocation *aAllocation)
{
    LOG(("size_allocate [%p] %d %d %d %d\n",
         (void *)this, aAllocation->x, aAllocation->y,
         aAllocation->width, aAllocation->height));

    nsIntSize size = GdkRectToDevicePixels(*aAllocation).Size();

    if (mBounds.Size() == size)
        return;

    nsIntRect rect;

    
    
    
    if (mBounds.width < size.width) {
        GdkRectangle rect = DevicePixelsToGdkRectRoundOut(
            { mBounds.width, 0, size.width - mBounds.width, size.height });
        gdk_window_invalidate_rect(mGdkWindow, &rect, FALSE);
    }
    if (mBounds.height < size.height) {
        GdkRectangle rect = DevicePixelsToGdkRectRoundOut(
            { 0, mBounds.height, size.width, size.height - mBounds.height });
        gdk_window_invalidate_rect(mGdkWindow, &rect, FALSE);
    }

    mBounds.SizeTo(size);

    if (!mGdkWindow)
        return;

    DispatchResized(size.width, size.height);
}

void
nsWindow::OnDeleteEvent()
{
    if (mWidgetListener)
        mWidgetListener->RequestWindowClose(this);
}

void
nsWindow::OnEnterNotifyEvent(GdkEventCrossing *aEvent)
{
    
    
    
    
    
    if (aEvent->subwindow != nullptr)
        return;

    
    
    DispatchMissedButtonReleases(aEvent);

    if (is_parent_ungrab_enter(aEvent))
        return;

    WidgetMouseEvent event(true, NS_MOUSE_ENTER, this, WidgetMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->x);
    event.refPoint.y = nscoord(aEvent->y);

    event.time = aEvent->time;

    LOG(("OnEnterNotify: %p\n", (void *)this));

    DispatchInputEvent(&event);
}


static bool
is_top_level_mouse_exit(GdkWindow* aWindow, GdkEventCrossing *aEvent)
{
    gint x = gint(aEvent->x_root);
    gint y = gint(aEvent->y_root);
    GdkDisplay* display = gdk_window_get_display(aWindow);
    GdkWindow* winAtPt = gdk_display_get_window_at_pointer(display, &x, &y);
    if (!winAtPt)
        return true;
    GdkWindow* topLevelAtPt = gdk_window_get_toplevel(winAtPt);
    GdkWindow* topLevelWidget = gdk_window_get_toplevel(aWindow);
    return topLevelAtPt != topLevelWidget;
}

void
nsWindow::OnLeaveNotifyEvent(GdkEventCrossing *aEvent)
{
    
    
    
    
    
    
    
    
    if (aEvent->subwindow != nullptr)
        return;

    WidgetMouseEvent event(true, NS_MOUSE_EXIT, this, WidgetMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->x);
    event.refPoint.y = nscoord(aEvent->y);

    event.time = aEvent->time;

    event.exit = is_top_level_mouse_exit(mGdkWindow, aEvent)
        ? WidgetMouseEvent::eTopLevel : WidgetMouseEvent::eChild;

    LOG(("OnLeaveNotify: %p\n", (void *)this));

    DispatchInputEvent(&event);
}

void
nsWindow::OnMotionNotifyEvent(GdkEventMotion *aEvent)
{
    
    
    
    bool synthEvent = false;
#ifdef MOZ_X11
    XEvent xevent;

    bool isX11Display = GDK_IS_X11_DISPLAY(gdk_display_get_default());
    if (isX11Display) {
        while (XPending (GDK_WINDOW_XDISPLAY(aEvent->window))) {
            XEvent peeked;
            XPeekEvent (GDK_WINDOW_XDISPLAY(aEvent->window), &peeked);
            if (peeked.xany.window != gdk_x11_window_get_xid(aEvent->window)
                || peeked.type != MotionNotify)
                break;

            synthEvent = true;
            XNextEvent (GDK_WINDOW_XDISPLAY(aEvent->window), &xevent);
        }
#if (MOZ_WIDGET_GTK == 2)
        
        if (gPluginFocusWindow && gPluginFocusWindow != this) {
            nsRefPtr<nsWindow> kungFuDeathGrip = gPluginFocusWindow;
            gPluginFocusWindow->LoseNonXEmbedPluginFocus();
        }
#endif 
    }
#endif 

    WidgetMouseEvent event(true, NS_MOUSE_MOVE, this, WidgetMouseEvent::eReal);

    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    
    
    if (pressure)
      mLastMotionPressure = pressure;
    event.pressure = mLastMotionPressure;

    guint modifierState;
    if (synthEvent) {
#ifdef MOZ_X11
        event.refPoint.x = nscoord(xevent.xmotion.x);
        event.refPoint.y = nscoord(xevent.xmotion.y);

        modifierState = xevent.xmotion.state;

        event.time = xevent.xmotion.time;
#else
        event.refPoint.x = nscoord(aEvent->x);
        event.refPoint.y = nscoord(aEvent->y);

        modifierState = aEvent->state;

        event.time = aEvent->time;
#endif 
    }
    else {
        
        if (aEvent->window == mGdkWindow) {
            event.refPoint.x = nscoord(aEvent->x);
            event.refPoint.y = nscoord(aEvent->y);
        } else {
            LayoutDeviceIntPoint point(NSToIntFloor(aEvent->x_root),
                                       NSToIntFloor(aEvent->y_root));
            event.refPoint = point - WidgetToScreenOffset();
        }

        modifierState = aEvent->state;

        event.time = aEvent->time;
    }

    KeymapWrapper::InitInputEvent(event, modifierState);

    DispatchInputEvent(&event);
}








void
nsWindow::DispatchMissedButtonReleases(GdkEventCrossing *aGdkEvent)
{
    guint changed = aGdkEvent->state ^ gButtonState;
    
    
    guint released = changed & gButtonState;
    gButtonState = aGdkEvent->state;

    
    
    for (guint buttonMask = GDK_BUTTON1_MASK;
         buttonMask <= GDK_BUTTON3_MASK;
         buttonMask <<= 1) {

        if (released & buttonMask) {
            int16_t buttonType;
            switch (buttonMask) {
            case GDK_BUTTON1_MASK:
                buttonType = WidgetMouseEvent::eLeftButton;
                break;
            case GDK_BUTTON2_MASK:
                buttonType = WidgetMouseEvent::eMiddleButton;
                break;
            default:
                NS_ASSERTION(buttonMask == GDK_BUTTON3_MASK,
                             "Unexpected button mask");
                buttonType = WidgetMouseEvent::eRightButton;
            }

            LOG(("Synthesized button %u release on %p\n",
                 guint(buttonType + 1), (void *)this));

            
            
            
            
            WidgetMouseEvent synthEvent(true, NS_MOUSE_BUTTON_UP, this,
                                        WidgetMouseEvent::eSynthesized);
            synthEvent.button = buttonType;
            DispatchInputEvent(&synthEvent);
        }
    }
}

void
nsWindow::InitButtonEvent(WidgetMouseEvent& aEvent,
                          GdkEventButton* aGdkEvent)
{
    
    if (aGdkEvent->window == mGdkWindow) {
        aEvent.refPoint.x = nscoord(aGdkEvent->x);
        aEvent.refPoint.y = nscoord(aGdkEvent->y);
    } else {
        LayoutDeviceIntPoint point(NSToIntFloor(aGdkEvent->x_root),
                                   NSToIntFloor(aGdkEvent->y_root));
        aEvent.refPoint = point - WidgetToScreenOffset();
    }

    guint modifierState = aGdkEvent->state;
    
    
    
    
    
    if (aGdkEvent->type != GDK_BUTTON_RELEASE) {
        switch (aGdkEvent->button) {
            case 1:
                modifierState |= GDK_BUTTON1_MASK;
                break;
            case 2:
                modifierState |= GDK_BUTTON2_MASK;
                break;
            case 3:
                modifierState |= GDK_BUTTON3_MASK;
                break;
        }
    }

    KeymapWrapper::InitInputEvent(aEvent, modifierState);

    aEvent.time = aGdkEvent->time;

    switch (aGdkEvent->type) {
    case GDK_2BUTTON_PRESS:
        aEvent.clickCount = 2;
        break;
    case GDK_3BUTTON_PRESS:
        aEvent.clickCount = 3;
        break;
        
    default:
        aEvent.clickCount = 1;
    }
}

static guint ButtonMaskFromGDKButton(guint button)
{
    return GDK_BUTTON1_MASK << (button - 1);
}

void
nsWindow::OnButtonPressEvent(GdkEventButton *aEvent)
{
    LOG(("Button %u press on %p\n", aEvent->button, (void *)this));

    
    
    
    
    
    
    GdkEvent *peekedEvent = gdk_event_peek();
    if (peekedEvent) {
        GdkEventType type = peekedEvent->any.type;
        gdk_event_free(peekedEvent);
        if (type == GDK_2BUTTON_PRESS || type == GDK_3BUTTON_PRESS)
            return;
    }

    nsWindow *containerWindow = GetContainerWindow();
    if (!gFocusWindow && containerWindow) {
        containerWindow->DispatchActivateEvent();
    }

    
    if (CheckForRollup(aEvent->x_root, aEvent->y_root, false, false))
        return;

    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    mLastMotionPressure = pressure;

    uint16_t domButton;
    switch (aEvent->button) {
    case 1:
        domButton = WidgetMouseEvent::eLeftButton;
        break;
    case 2:
        domButton = WidgetMouseEvent::eMiddleButton;
        break;
    case 3:
        domButton = WidgetMouseEvent::eRightButton;
        break;
    
    case 6:
    case 7:
        NS_WARNING("We're not supporting legacy horizontal scroll event");
        return;
    
    case 8:
        DispatchCommandEvent(nsGkAtoms::Back);
        return;
    case 9:
        DispatchCommandEvent(nsGkAtoms::Forward);
        return;
    default:
        return;
    }

    gButtonState |= ButtonMaskFromGDKButton(aEvent->button);

    WidgetMouseEvent event(true, NS_MOUSE_BUTTON_DOWN, this,
                           WidgetMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent);
    event.pressure = mLastMotionPressure;

    DispatchInputEvent(&event);

    
    if (domButton == WidgetMouseEvent::eRightButton &&
        MOZ_LIKELY(!mIsDestroyed)) {
        WidgetMouseEvent contextMenuEvent(true, NS_CONTEXTMENU, this,
                                          WidgetMouseEvent::eReal);
        InitButtonEvent(contextMenuEvent, aEvent);
        contextMenuEvent.pressure = mLastMotionPressure;
        DispatchInputEvent(&contextMenuEvent);
    }
}

void
nsWindow::OnButtonReleaseEvent(GdkEventButton *aEvent)
{
    LOG(("Button %u release on %p\n", aEvent->button, (void *)this));

    uint16_t domButton;
    switch (aEvent->button) {
    case 1:
        domButton = WidgetMouseEvent::eLeftButton;
        break;
    case 2:
        domButton = WidgetMouseEvent::eMiddleButton;
        break;
    case 3:
        domButton = WidgetMouseEvent::eRightButton;
        break;
    default:
        return;
    }

    gButtonState &= ~ButtonMaskFromGDKButton(aEvent->button);

    WidgetMouseEvent event(true, NS_MOUSE_BUTTON_UP, this,
                           WidgetMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent);
    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    event.pressure = pressure ? pressure : mLastMotionPressure;

    DispatchInputEvent(&event);
    mLastMotionPressure = pressure;
}

void
nsWindow::OnContainerFocusInEvent(GdkEventFocus *aEvent)
{
    LOGFOCUS(("OnContainerFocusInEvent [%p]\n", (void *)this));

    
    GtkWidget* top_window = GetToplevelWidget();
    if (top_window && (gtk_widget_get_visible(top_window)))
        SetUrgencyHint(top_window, false);

    
    
    if (gBlockActivateEvent) {
        LOGFOCUS(("activated notification is blocked [%p]\n", (void *)this));
        return;
    }

    
    
    gFocusWindow = nullptr;

    DispatchActivateEvent();

    if (!gFocusWindow) {
        
        
        
        
        gFocusWindow = this;
    }

    LOGFOCUS(("Events sent from focus in event [%p]\n", (void *)this));
}

void
nsWindow::OnContainerFocusOutEvent(GdkEventFocus *aEvent)
{
    LOGFOCUS(("OnContainerFocusOutEvent [%p]\n", (void *)this));

    if (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog) {
        nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
        nsCOMPtr<nsIDragSession> dragSession;
        dragService->GetCurrentSession(getter_AddRefs(dragSession));

        
        
        
        bool shouldRollup = !dragSession;
        if (!shouldRollup) {
            
            nsCOMPtr<nsIDOMNode> sourceNode;
            dragSession->GetSourceNode(getter_AddRefs(sourceNode));
            shouldRollup = (sourceNode == nullptr);
        }

        if (shouldRollup) {
            CheckForRollup(0, 0, false, true);
        }
    }

#if (MOZ_WIDGET_GTK == 2) && defined(MOZ_X11)
    
    if (gPluginFocusWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gPluginFocusWindow;
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }
#endif 

    if (gFocusWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gFocusWindow;
        if (gFocusWindow->mIMModule) {
            gFocusWindow->mIMModule->OnBlurWindow(gFocusWindow);
        }
        gFocusWindow = nullptr;
    }

    DispatchDeactivateEvent();

    LOGFOCUS(("Done with container focus out [%p]\n", (void *)this));
}

bool
nsWindow::DispatchCommandEvent(nsIAtom* aCommand)
{
    nsEventStatus status;
    WidgetCommandEvent event(true, nsGkAtoms::onAppCommand, aCommand, this);
    DispatchEvent(&event, status);
    return TRUE;
}

bool
nsWindow::DispatchContentCommandEvent(int32_t aMsg)
{
  nsEventStatus status;
  WidgetContentCommandEvent event(true, aMsg, this);
  DispatchEvent(&event, status);
  return TRUE;
}

static bool
IsCtrlAltTab(GdkEventKey *aEvent)
{
    return aEvent->keyval == GDK_Tab &&
        KeymapWrapper::AreModifiersActive(
            KeymapWrapper::CTRL | KeymapWrapper::ALT, aEvent->state);
}

bool
nsWindow::DispatchKeyDownEvent(GdkEventKey *aEvent, bool *aCancelled)
{
    NS_PRECONDITION(aCancelled, "aCancelled must not be null");

    *aCancelled = false;

    if (IsCtrlAltTab(aEvent)) {
        return false;
    }

    
    WidgetKeyboardEvent downEvent(true, NS_KEY_DOWN, this);
    KeymapWrapper::InitKeyEvent(downEvent, aEvent);
    nsEventStatus status = DispatchInputEvent(&downEvent);
    *aCancelled = (status == nsEventStatus_eConsumeNoDefault);
    return true;
}

gboolean
nsWindow::OnKeyPressEvent(GdkEventKey *aEvent)
{
    LOGFOCUS(("OnKeyPressEvent [%p]\n", (void *)this));

    
    
    bool IMEWasEnabled = false;
    if (mIMModule) {
        IMEWasEnabled = mIMModule->IsEnabled();
        if (mIMModule->OnKeyEvent(this, aEvent)) {
            return TRUE;
        }
    }

    nsEventStatus status;

    
    if (IsCtrlAltTab(aEvent)) {
        return TRUE;
    }

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;

    
    
    
    
    

    bool isKeyDownCancelled = false;
    if (DispatchKeyDownEvent(aEvent, &isKeyDownCancelled) &&
        (MOZ_UNLIKELY(mIsDestroyed) || isKeyDownCancelled)) {
        return TRUE;
    }

    
    
    
    if (!IMEWasEnabled && mIMModule && mIMModule->IsEnabled()) {
        
        
        if (mIMModule->OnKeyEvent(this, aEvent, true)) {
            return TRUE;
        }
    }

    
    
    
    
    if (!KeymapWrapper::IsKeyPressEventNecessary(aEvent)) {
        return TRUE;
    }

#ifdef MOZ_X11
#if ! defined AIX 
    
    switch (aEvent->keyval) {
        case XF86XK_Back:
            return DispatchCommandEvent(nsGkAtoms::Back);
        case XF86XK_Forward:
            return DispatchCommandEvent(nsGkAtoms::Forward);
        case XF86XK_Refresh:
            return DispatchCommandEvent(nsGkAtoms::Reload);
        case XF86XK_Stop:
            return DispatchCommandEvent(nsGkAtoms::Stop);
        case XF86XK_Search:
            return DispatchCommandEvent(nsGkAtoms::Search);
        case XF86XK_Favorites:
            return DispatchCommandEvent(nsGkAtoms::Bookmarks);
        case XF86XK_HomePage:
            return DispatchCommandEvent(nsGkAtoms::Home);
        case XF86XK_Copy:
        case GDK_F16:  
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_COPY);
        case XF86XK_Cut:
        case GDK_F20:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_CUT);
        case XF86XK_Paste:
        case GDK_F18:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_PASTE);
        case GDK_Redo:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_REDO);
        case GDK_Undo:
        case GDK_F14:
            return DispatchContentCommandEvent(NS_CONTENT_COMMAND_UNDO);
    }
#endif 
#endif 

    WidgetKeyboardEvent event(true, NS_KEY_PRESS, this);
    KeymapWrapper::InitKeyEvent(event, aEvent);

    
    
    if (is_context_menu_key(event)) {
        WidgetMouseEvent contextMenuEvent(true, NS_CONTEXTMENU, this,
                                          WidgetMouseEvent::eReal,
                                          WidgetMouseEvent::eContextMenuKey);

        contextMenuEvent.refPoint = LayoutDeviceIntPoint(0, 0);
        contextMenuEvent.time = aEvent->time;
        contextMenuEvent.clickCount = 1;
        KeymapWrapper::InitInputEvent(contextMenuEvent, aEvent->state);
        status = DispatchInputEvent(&contextMenuEvent);
    }
    else {
        
        
        
        if (IS_IN_BMP(event.charCode)) {
            status = DispatchInputEvent(&event);
        }
        else {
            WidgetCompositionEvent compositionChangeEvent(
                                     true, NS_COMPOSITION_CHANGE, this);
            char16_t textString[3];
            textString[0] = H_SURROGATE(event.charCode);
            textString[1] = L_SURROGATE(event.charCode);
            textString[2] = 0;
            compositionChangeEvent.mData = textString;
            compositionChangeEvent.time = event.time;
            DispatchEvent(&compositionChangeEvent, status);
        }
    }

    
    if (status == nsEventStatus_eConsumeNoDefault) {
        return TRUE;
    }

    return FALSE;
}

gboolean
nsWindow::OnKeyReleaseEvent(GdkEventKey *aEvent)
{
    LOGFOCUS(("OnKeyReleaseEvent [%p]\n", (void *)this));

    if (mIMModule && mIMModule->OnKeyEvent(this, aEvent)) {
        return TRUE;
    }

    
    WidgetKeyboardEvent event(true, NS_KEY_UP, this);
    KeymapWrapper::InitKeyEvent(event, aEvent);

    nsEventStatus status = DispatchInputEvent(&event);

    
    if (status == nsEventStatus_eConsumeNoDefault) {
        return TRUE;
    }

    return FALSE;
}

void
nsWindow::OnScrollEvent(GdkEventScroll *aEvent)
{
    
    if (CheckForRollup(aEvent->x_root, aEvent->y_root, true, false))
        return;
#if GTK_CHECK_VERSION(3,4,0)
    
    if (mLastScrollEventTime == aEvent->time)
        return; 
#endif
    WidgetWheelEvent wheelEvent(true, NS_WHEEL_WHEEL, this);
    wheelEvent.deltaMode = nsIDOMWheelEvent::DOM_DELTA_LINE;
    switch (aEvent->direction) {
#if GTK_CHECK_VERSION(3,4,0)
    case GDK_SCROLL_SMOOTH:
    {
        
        
        mLastScrollEventTime = aEvent->time;
        
        
        wheelEvent.deltaX = aEvent->delta_x * 3;
        wheelEvent.deltaY = aEvent->delta_y * 3;
        wheelEvent.mIsNoLineOrPageDelta = true;
        
        
        
        GdkDevice *device = gdk_event_get_source_device((GdkEvent*)aEvent);
        GdkInputSource source = gdk_device_get_source(device);
        if (source == GDK_SOURCE_TOUCHSCREEN ||
            source == GDK_SOURCE_TOUCHPAD) {
            wheelEvent.scrollType = WidgetWheelEvent::SCROLL_ASYNCHRONOUSELY;
        }
        break;
    }
#endif
    case GDK_SCROLL_UP:
        wheelEvent.deltaY = wheelEvent.lineOrPageDeltaY = -3;
        break;
    case GDK_SCROLL_DOWN:
        wheelEvent.deltaY = wheelEvent.lineOrPageDeltaY = 3;
        break;
    case GDK_SCROLL_LEFT:
        wheelEvent.deltaX = wheelEvent.lineOrPageDeltaX = -1;
        break;
    case GDK_SCROLL_RIGHT:
        wheelEvent.deltaX = wheelEvent.lineOrPageDeltaX = 1;
        break;
    }

    NS_ASSERTION(wheelEvent.deltaX || wheelEvent.deltaY,
                 "deltaX or deltaY must be non-zero");

    if (aEvent->window == mGdkWindow) {
        
        wheelEvent.refPoint.x = nscoord(aEvent->x);
        wheelEvent.refPoint.y = nscoord(aEvent->y);
    } else {
        
        
        
        LayoutDeviceIntPoint point(NSToIntFloor(aEvent->x_root),
                                   NSToIntFloor(aEvent->y_root));
        wheelEvent.refPoint = point - WidgetToScreenOffset();
    }

    KeymapWrapper::InitInputEvent(wheelEvent, aEvent->state);

    wheelEvent.time = aEvent->time;

    DispatchAPZAwareEvent(&wheelEvent);
}

void
nsWindow::OnVisibilityNotifyEvent(GdkEventVisibility *aEvent)
{
    LOGDRAW(("Visibility event %i on [%p] %p\n",
             aEvent->state, this, aEvent->window));

    if (!mGdkWindow)
        return;

    switch (aEvent->state) {
    case GDK_VISIBILITY_UNOBSCURED:
    case GDK_VISIBILITY_PARTIAL:
        if (mIsFullyObscured && mHasMappedToplevel) {
            
            
            gdk_window_invalidate_rect(mGdkWindow, nullptr, FALSE);
        }

        mIsFullyObscured = false;

        
        EnsureGrabs();
        break;
    default: 
        mIsFullyObscured = true;
        break;
    }
}

void
nsWindow::OnWindowStateEvent(GtkWidget *aWidget, GdkEventWindowState *aEvent)
{
    LOG(("nsWindow::OnWindowStateEvent [%p] changed %d new_window_state %d\n",
         (void *)this, aEvent->changed_mask, aEvent->new_window_state));

    if (IS_MOZ_CONTAINER(aWidget)) {
        
        
        
        
        
        
        
        
        
        
        bool mapped =
            !(aEvent->new_window_state &
              (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN));
        if (mHasMappedToplevel != mapped) {
            SetHasMappedToplevel(mapped);
        }
        return;
    }
    

    
    
    if ((aEvent->changed_mask
         & (GDK_WINDOW_STATE_ICONIFIED |
            GDK_WINDOW_STATE_MAXIMIZED |
            GDK_WINDOW_STATE_FULLSCREEN)) == 0) {
        return;
    }

    if (aEvent->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
        LOG(("\tIconified\n"));
        mSizeState = nsSizeMode_Minimized;
#ifdef ACCESSIBILITY
        DispatchMinimizeEventAccessible();
#endif 
    }
    else if (aEvent->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
        LOG(("\tFullscreen\n"));
        mSizeState = nsSizeMode_Fullscreen;
    }
    else if (aEvent->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        LOG(("\tMaximized\n"));
        mSizeState = nsSizeMode_Maximized;
#ifdef ACCESSIBILITY
        DispatchMaximizeEventAccessible();
#endif 
    }
    else {
        LOG(("\tNormal\n"));
        mSizeState = nsSizeMode_Normal;
#ifdef ACCESSIBILITY
        DispatchRestoreEventAccessible();
#endif 
    }

    if (mWidgetListener)
      mWidgetListener->SizeModeChanged(mSizeState);
}

void
nsWindow::ThemeChanged()
{
    NotifyThemeChanged();

    if (!mGdkWindow || MOZ_UNLIKELY(mIsDestroyed))
        return;

    
    GList *children =
        gdk_window_peek_children(mGdkWindow);
    while (children) {
        GdkWindow *gdkWin = GDK_WINDOW(children->data);

        nsWindow *win = (nsWindow*) g_object_get_data(G_OBJECT(gdkWin),
                                                      "nsWindow");

        if (win && win != this) { 
            nsRefPtr<nsWindow> kungFuDeathGrip = win;
            win->ThemeChanged();
        }

        children = children->next;
    }
}

void
nsWindow::DispatchDragEvent(uint32_t aMsg, const nsIntPoint& aRefPoint,
                            guint aTime)
{
    WidgetDragEvent event(true, aMsg, this);

    if (aMsg == NS_DRAGDROP_OVER) {
        InitDragEvent(event);
    }

    event.refPoint = LayoutDeviceIntPoint::FromUntyped(aRefPoint);
    event.time = aTime;

    DispatchInputEvent(&event);
}

void
nsWindow::OnDragDataReceivedEvent(GtkWidget *aWidget,
                                  GdkDragContext *aDragContext,
                                  gint aX,
                                  gint aY,
                                  GtkSelectionData  *aSelectionData,
                                  guint aInfo,
                                  guint aTime,
                                  gpointer aData)
{
    LOGDRAG(("nsWindow::OnDragDataReceived(%p)\n", (void*)this));

    nsDragService::GetInstance()->
        TargetDataReceived(aWidget, aDragContext, aX, aY,
                           aSelectionData, aInfo, aTime);
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
            MOZ_UTF16("brandShortName"),
            getter_Copies(brandName));

    if (brandName.IsEmpty())
        brandName.AssignLiteral(MOZ_UTF16("Mozilla"));
}

static GdkWindow *
CreateGdkWindow(GdkWindow *parent, GtkWidget *widget)
{
    GdkWindowAttr attributes;
    gint          attributes_mask = GDK_WA_VISUAL;

    attributes.event_mask = kEvents;

    attributes.width = 1;
    attributes.height = 1;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.window_type = GDK_WINDOW_CHILD;

#if (MOZ_WIDGET_GTK == 2)
    attributes_mask |= GDK_WA_COLORMAP;
    attributes.colormap = gtk_widget_get_colormap(widget);
#endif

    GdkWindow *window = gdk_window_new(parent, &attributes, attributes_mask);
    gdk_window_set_user_data(window, widget);


#if (MOZ_WIDGET_GTK == 2)
    

    gdk_window_set_back_pixmap(window, nullptr, FALSE);
#endif

    return window;
}

nsresult
nsWindow::Create(nsIWidget        *aParent,
                 nsNativeWidget    aNativeParent,
                 const nsIntRect  &aRect,
                 nsWidgetInitData *aInitData)
{
    
    
    nsIWidget *baseParent = aInitData &&
        (aInitData->mWindowType == eWindowType_dialog ||
         aInitData->mWindowType == eWindowType_toplevel ||
         aInitData->mWindowType == eWindowType_invisible) ?
        nullptr : aParent;

#ifdef ACCESSIBILITY
    
    a11y::PreInit();
#endif

    
    nsGTKToolkit::GetToolkit();

    
    BaseCreate(baseParent, aRect, aInitData);

    
    bool listenForResizes = false;;
    if (aNativeParent || (aInitData && aInitData->mListenForResizes))
        listenForResizes = true;

    
    CommonCreate(aParent, listenForResizes);

    
    mBounds = aRect;
    ConstrainSize(&mBounds.width, &mBounds.height);

    
    GtkWidget      *parentMozContainer = nullptr;
    GtkContainer   *parentGtkContainer = nullptr;
    GdkWindow      *parentGdkWindow = nullptr;
    GtkWindow      *topLevelParent = nullptr;
    nsWindow       *parentnsWindow = nullptr;
    GtkWidget      *eventWidget = nullptr;

    if (aParent) {
        parentnsWindow = static_cast<nsWindow*>(aParent);
        parentGdkWindow = parentnsWindow->mGdkWindow;
    } else if (aNativeParent && GDK_IS_WINDOW(aNativeParent)) {
        parentGdkWindow = GDK_WINDOW(aNativeParent);
        parentnsWindow = get_window_for_gdk_window(parentGdkWindow);
        if (!parentnsWindow)
            return NS_ERROR_FAILURE;

    } else if (aNativeParent && GTK_IS_CONTAINER(aNativeParent)) {
        parentGtkContainer = GTK_CONTAINER(aNativeParent);
    }

    if (parentGdkWindow) {
        
        parentMozContainer = parentnsWindow->GetMozContainerWidget();
        if (!parentMozContainer)
            return NS_ERROR_FAILURE;

        
        
        topLevelParent =
            GTK_WINDOW(gtk_widget_get_toplevel(parentMozContainer));
    }

    
    switch (mWindowType) {
    case eWindowType_dialog:
    case eWindowType_popup:
    case eWindowType_toplevel:
    case eWindowType_invisible: {
        mIsTopLevel = true;

        
        
        
        
        
        
        mNeedsResize = true;

        if (mWindowType == eWindowType_dialog) {
            mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            SetDefaultIcon();
            gtk_window_set_wmclass(GTK_WINDOW(mShell), "Dialog", 
                                   gdk_get_program_class());
            gtk_window_set_type_hint(GTK_WINDOW(mShell),
                                     GDK_WINDOW_TYPE_HINT_DIALOG);
            gtk_window_set_transient_for(GTK_WINDOW(mShell),
                                         topLevelParent);
        }
        else if (mWindowType == eWindowType_popup) {
            
            
            
            mNeedsMove = true;

            
            
            
            
            
            
            
            GtkWindowType type = aInitData->mNoAutoHide ?
                                     GTK_WINDOW_TOPLEVEL : GTK_WINDOW_POPUP;
            mShell = gtk_window_new(type);
            gtk_window_set_wmclass(GTK_WINDOW(mShell), "Popup",
                                   gdk_get_program_class());

            if (aInitData->mSupportTranslucency) {
                
                
                
                
                GdkScreen *screen = gtk_widget_get_screen(mShell);
                if (gdk_screen_is_composited(screen)) {
#if (MOZ_WIDGET_GTK == 2)
                    GdkColormap *colormap =
                        gdk_screen_get_rgba_colormap(screen);
                    gtk_widget_set_colormap(mShell, colormap);
#else
                    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
                    gtk_widget_set_visual(mShell, visual);
#endif
                }
            }
            if (aInitData->mNoAutoHide) {
                
                
                if (mBorderStyle == eBorderStyle_default) {
                  gtk_window_set_decorated(GTK_WINDOW(mShell), FALSE);
                }
                else {
                  bool decorate = mBorderStyle & eBorderStyle_title;
                  gtk_window_set_decorated(GTK_WINDOW(mShell), decorate);
                  if (decorate) {
                    gtk_window_set_deletable(GTK_WINDOW(mShell), mBorderStyle & eBorderStyle_close);
                  }
                }
                gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mShell), TRUE);
                
                
                
                gtk_window_set_accept_focus(GTK_WINDOW(mShell), FALSE);
#ifdef MOZ_X11
                
                
                gtk_widget_realize(mShell);
                gdk_window_add_filter(gtk_widget_get_window(mShell),
                                      popup_take_focus_filter, nullptr); 
#endif
            }

            GdkWindowTypeHint gtkTypeHint;
            if (aInitData->mIsDragPopup) {
                gtkTypeHint = GDK_WINDOW_TYPE_HINT_DND;
            }
            else {
                switch (aInitData->mPopupHint) {
                    case ePopupTypeMenu:
                        gtkTypeHint = GDK_WINDOW_TYPE_HINT_POPUP_MENU;
                        break;
                    case ePopupTypeTooltip:
                        gtkTypeHint = GDK_WINDOW_TYPE_HINT_TOOLTIP;
                        break;
                    default:
                        gtkTypeHint = GDK_WINDOW_TYPE_HINT_UTILITY;
                        break;
                }
            }
            gtk_window_set_type_hint(GTK_WINDOW(mShell), gtkTypeHint);

            if (topLevelParent) {
                gtk_window_set_transient_for(GTK_WINDOW(mShell),
                                            topLevelParent);
            }
        }
        else { 
            mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            SetDefaultIcon();
            gtk_window_set_wmclass(GTK_WINDOW(mShell), "Toplevel", 
                                   gdk_get_program_class());

            
            GtkWindowGroup *group = gtk_window_group_new();
            gtk_window_group_add_window(group, GTK_WINDOW(mShell));
            g_object_unref(group);
        }

        
        gtk_widget_set_app_paintable(mShell, TRUE);

        
        GtkWidget *container = moz_container_new();
        mContainer = MOZ_CONTAINER(container);
        
        gtk_widget_set_has_window(container, FALSE);
        eventWidget = mShell;
        gtk_widget_add_events(eventWidget, kEvents);
        gtk_container_add(GTK_CONTAINER(mShell), container);
        gtk_widget_realize(container);

        
        gtk_widget_show(container);
        gtk_widget_grab_focus(container);

        
        mGdkWindow = gtk_widget_get_window(mShell);

        if (mWindowType == eWindowType_popup) {
            
            

            mCursor = eCursor_wait; 
                                    
                                    
                                    
            SetCursor(eCursor_standard);

            if (aInitData->mNoAutoHide) {
                gint wmd = ConvertBorderStyles(mBorderStyle);
                if (wmd != -1)
                  gdk_window_set_decorations(gtk_widget_get_window(mShell), (GdkWMDecoration) wmd);
            }

            
            if (aInitData->mMouseTransparent) {
#if (MOZ_WIDGET_GTK == 2)
              GdkRectangle rect = { 0, 0, 0, 0 };
              GdkRegion *region = gdk_region_rectangle(&rect);

              gdk_window_input_shape_combine_region(mGdkWindow, region, 0, 0);
              gdk_region_destroy(region);
#else
              cairo_rectangle_int_t rect = { 0, 0, 0, 0 };
              cairo_region_t *region = cairo_region_create_rectangle(&rect);

              gdk_window_input_shape_combine_region(mGdkWindow, region, 0, 0);
              cairo_region_destroy(region);
#endif
            }
        }
    }
        break;
    case eWindowType_plugin:
    case eWindowType_plugin_ipc_chrome:
    case eWindowType_plugin_ipc_content:
    case eWindowType_child: {
        if (parentMozContainer) {
            mGdkWindow = CreateGdkWindow(parentGdkWindow, parentMozContainer);
            mHasMappedToplevel = parentnsWindow->mHasMappedToplevel;
        }
        else if (parentGtkContainer) {
            
            
            
            GtkWidget *container = moz_container_new();
            mContainer = MOZ_CONTAINER(container);
            eventWidget = container;
            gtk_widget_add_events(eventWidget, kEvents);
            gtk_container_add(parentGtkContainer, container);
            gtk_widget_realize(container);

            mGdkWindow = gtk_widget_get_window(container);
        }
        else {
            NS_WARNING("Warning: tried to create a new child widget with no parent!");
            return NS_ERROR_FAILURE;
        }
    }
        break;
    default:
        break;
    }

    
    g_object_set_data(G_OBJECT(mGdkWindow), "nsWindow", this);

    if (mContainer)
        g_object_set_data(G_OBJECT(mContainer), "nsWindow", this);

    if (mShell)
        g_object_set_data(G_OBJECT(mShell), "nsWindow", this);

    
    if (mShell) {
        g_signal_connect(mShell, "configure_event",
                         G_CALLBACK(configure_event_cb), nullptr);
        g_signal_connect(mShell, "delete_event",
                         G_CALLBACK(delete_event_cb), nullptr);
        g_signal_connect(mShell, "window_state_event",
                         G_CALLBACK(window_state_event_cb), nullptr);

        GtkSettings* default_settings = gtk_settings_get_default();
        g_signal_connect_after(default_settings,
                               "notify::gtk-theme-name",
                               G_CALLBACK(theme_changed_cb), this);
        g_signal_connect_after(default_settings,
                               "notify::gtk-font-name",
                               G_CALLBACK(theme_changed_cb), this);
    }

    if (mContainer) {
        
        g_signal_connect(mContainer, "unrealize",
                         G_CALLBACK(container_unrealize_cb), nullptr);
        g_signal_connect_after(mContainer, "size_allocate",
                               G_CALLBACK(size_allocate_cb), nullptr);
        g_signal_connect(mContainer, "hierarchy-changed",
                         G_CALLBACK(hierarchy_changed_cb), nullptr);
        
        hierarchy_changed_cb(GTK_WIDGET(mContainer), nullptr);
        
        
#if (MOZ_WIDGET_GTK == 2)
        g_signal_connect(mContainer, "expose_event",
                         G_CALLBACK(expose_event_cb), nullptr);
#else
        g_signal_connect(G_OBJECT(mContainer), "draw",
                         G_CALLBACK(expose_event_cb), nullptr);
#endif
        g_signal_connect(mContainer, "focus_in_event",
                         G_CALLBACK(focus_in_event_cb), nullptr);
        g_signal_connect(mContainer, "focus_out_event",
                         G_CALLBACK(focus_out_event_cb), nullptr);
        g_signal_connect(mContainer, "key_press_event",
                         G_CALLBACK(key_press_event_cb), nullptr);
        g_signal_connect(mContainer, "key_release_event",
                         G_CALLBACK(key_release_event_cb), nullptr);

        gtk_drag_dest_set((GtkWidget *)mContainer,
                          (GtkDestDefaults)0,
                          nullptr,
                          0,
                          (GdkDragAction)0);

        g_signal_connect(mContainer, "drag_motion",
                         G_CALLBACK(drag_motion_event_cb), nullptr);
        g_signal_connect(mContainer, "drag_leave",
                         G_CALLBACK(drag_leave_event_cb), nullptr);
        g_signal_connect(mContainer, "drag_drop",
                         G_CALLBACK(drag_drop_event_cb), nullptr);
        g_signal_connect(mContainer, "drag_data_received",
                         G_CALLBACK(drag_data_received_event_cb), nullptr);

        GtkWidget *widgets[] = { GTK_WIDGET(mContainer), mShell };
        for (size_t i = 0; i < ArrayLength(widgets) && widgets[i]; ++i) {
            
            
            
            g_signal_connect(widgets[i], "visibility-notify-event",
                             G_CALLBACK(visibility_notify_event_cb), nullptr);
            
            
            
            gtk_widget_set_double_buffered(widgets[i], FALSE);
        }

        
        
        if (mWindowType != eWindowType_popup) {
            mIMModule = new nsGtkIMModule(this);
        }
    } else if (!mIMModule) {
        nsWindow *container = GetContainerWindow();
        if (container) {
            mIMModule = container->mIMModule;
        }
    }

    if (eventWidget) {
#if (MOZ_WIDGET_GTK == 2)
        
        GTK_PRIVATE_SET_FLAG(eventWidget, GTK_HAS_SHAPE_MASK);
#endif

        
        
        
        
        g_signal_connect(eventWidget, "enter-notify-event",
                         G_CALLBACK(enter_notify_event_cb), nullptr);
        g_signal_connect(eventWidget, "leave-notify-event",
                         G_CALLBACK(leave_notify_event_cb), nullptr);
        g_signal_connect(eventWidget, "motion-notify-event",
                         G_CALLBACK(motion_notify_event_cb), nullptr);
        g_signal_connect(eventWidget, "button-press-event",
                         G_CALLBACK(button_press_event_cb), nullptr);
        g_signal_connect(eventWidget, "button-release-event",
                         G_CALLBACK(button_release_event_cb), nullptr);
        g_signal_connect(eventWidget, "scroll-event",
                         G_CALLBACK(scroll_event_cb), nullptr);
    }

    LOG(("nsWindow [%p]\n", (void *)this));
    if (mShell) {
        LOG(("\tmShell %p mContainer %p mGdkWindow %p 0x%lx\n",
             mShell, mContainer, mGdkWindow,
             gdk_x11_window_get_xid(mGdkWindow)));
    } else if (mContainer) {
        LOG(("\tmContainer %p mGdkWindow %p\n", mContainer, mGdkWindow));
    }
    else if (mGdkWindow) {
        LOG(("\tmGdkWindow %p parent %p\n",
             mGdkWindow, gdk_window_get_parent(mGdkWindow)));
    }

    
    if (!mIsTopLevel)
        Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, false);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString &xulWinType)
{
  if (!mShell)
    return NS_ERROR_FAILURE;

  const char *res_class = gdk_get_program_class();
  if (!res_class)
    return NS_ERROR_FAILURE;
  
  char *res_name = ToNewCString(xulWinType);
  if (!res_name)
    return NS_ERROR_OUT_OF_MEMORY;

  const char *role = nullptr;

  
  
  
  
  for (char *c = res_name; *c; c++) {
    if (':' == *c) {
      *c = 0;
      role = c + 1;
    }
    else if (!isascii(*c) || (!isalnum(*c) && ('_' != *c) && ('-' != *c)))
      *c = '_';
  }
  res_name[0] = toupper(res_name[0]);
  if (!role) role = res_name;

  GdkWindow *shellWindow = gtk_widget_get_window(GTK_WIDGET(mShell));
  gdk_window_set_role(shellWindow, role);

#ifdef MOZ_X11
  GdkDisplay *display = gdk_display_get_default();
  if (GDK_IS_X11_DISPLAY(display)) {
      XClassHint *class_hint = XAllocClassHint();
      if (!class_hint) {
        free(res_name);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      class_hint->res_name = res_name;
      class_hint->res_class = const_cast<char*>(res_class);

      
      
      XSetClassHint(GDK_DISPLAY_XDISPLAY(display),
                    gdk_x11_window_get_xid(shellWindow),
                    class_hint);
      XFree(class_hint);
  }
#endif 

  free(res_name);

  return NS_OK;
}

void
nsWindow::NativeResize(int32_t aWidth, int32_t aHeight, bool    aRepaint)
{
    gint width = DevicePixelsToGdkCoordRoundUp(aWidth);
    gint height = DevicePixelsToGdkCoordRoundUp(aHeight);
    
    LOG(("nsWindow::NativeResize [%p] %d %d\n", (void *)this,
         width, height));

    
    mNeedsResize = false;

    if (mIsTopLevel) {
        gtk_window_resize(GTK_WINDOW(mShell), width, height);
    }
    else if (mContainer) {
        GtkWidget *widget = GTK_WIDGET(mContainer);
        GtkAllocation allocation, prev_allocation;
        gtk_widget_get_allocation(widget, &prev_allocation);
        allocation.x = prev_allocation.x;
        allocation.y = prev_allocation.y;
        allocation.width = width;
        allocation.height = height;
        gtk_widget_size_allocate(widget, &allocation);
    }
    else if (mGdkWindow) {
        gdk_window_resize(mGdkWindow, width, height);
    }
}

void
nsWindow::NativeResize(int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight,
                       bool    aRepaint)
{
    gint width = DevicePixelsToGdkCoordRoundUp(aWidth);
    gint height = DevicePixelsToGdkCoordRoundUp(aHeight);
    gint x = DevicePixelsToGdkCoordRoundDown(aX);
    gint y = DevicePixelsToGdkCoordRoundDown(aY);

    mNeedsResize = false;
    mNeedsMove = false;

    LOG(("nsWindow::NativeResize [%p] %d %d %d %d\n", (void *)this,
         x, y, width, height));

    if (mIsTopLevel) {
        
        gtk_window_move(GTK_WINDOW(mShell), x, y);
        
        gtk_window_resize(GTK_WINDOW(mShell), width, height);
    }
    else if (mContainer) {
        GtkAllocation allocation;
        allocation.x = x;
        allocation.y = y;
        allocation.width = width;
        allocation.height = height;
        gtk_widget_size_allocate(GTK_WIDGET(mContainer), &allocation);
    }
    else if (mGdkWindow) {
        gdk_window_move_resize(mGdkWindow, x, y, width, height);
    }
}

void
nsWindow::NativeShow(bool aAction)
{
    if (aAction) {
        
        mNeedsShow = false;

        if (mIsTopLevel) {
            
            if (mWindowType != eWindowType_invisible) {
                SetUserTimeAndStartupIDForActivatedWindow(mShell);
            }

            gtk_widget_show(mShell);
        }
        else if (mContainer) {
            gtk_widget_show(GTK_WIDGET(mContainer));
        }
        else if (mGdkWindow) {
            gdk_window_show_unraised(mGdkWindow);
        }
    }
    else {
        if (mIsTopLevel) {
            gtk_widget_hide(GTK_WIDGET(mShell));

            ClearTransparencyBitmap(); 
        }
        else if (mContainer) {
            gtk_widget_hide(GTK_WIDGET(mContainer));
        }
        else if (mGdkWindow) {
            gdk_window_hide(mGdkWindow);
        }
    }
}

void
nsWindow::SetHasMappedToplevel(bool aState)
{
    
    
    
    bool oldState = mHasMappedToplevel;
    mHasMappedToplevel = aState;

    
    
    
    
    
    
    
    if (!mIsShown || !mGdkWindow)
        return;

    if (aState && !oldState && !mIsFullyObscured) {
        
        
        
        gdk_window_invalidate_rect(mGdkWindow, nullptr, FALSE);

        
        
        EnsureGrabs();
    }

    for (GList *children = gdk_window_peek_children(mGdkWindow);
         children;
         children = children->next) {
        GdkWindow *gdkWin = GDK_WINDOW(children->data);
        nsWindow *child = get_window_for_gdk_window(gdkWin);

        if (child && child->mHasMappedToplevel != aState) {
            child->SetHasMappedToplevel(aState);
        }
    }
}

LayoutDeviceIntSize
nsWindow::GetSafeWindowSize(LayoutDeviceIntSize aSize)
{
    
    
    
    
    LayoutDeviceIntSize result = aSize;
    const int32_t kInt16Max = 32767;
    if (result.width > kInt16Max) {
        result.width = kInt16Max;
    }
    if (result.height > kInt16Max) {
        result.height = kInt16Max;
    }
    return result;
}

void
nsWindow::EnsureGrabs(void)
{
    if (mRetryPointerGrab)
        GrabPointer(sRetryGrabTime);
}

void
nsWindow::CleanLayerManagerRecursive(void) {
    if (mLayerManager) {
        mLayerManager->Destroy();
        mLayerManager = nullptr;
    }

    DestroyCompositor();

    GList* children = gdk_window_peek_children(mGdkWindow);
    for (GList* list = children; list; list = list->next) {
        nsWindow* window = get_window_for_gdk_window(GDK_WINDOW(list->data));
        if (window) {
            window->CleanLayerManagerRecursive();
        }
    }
}

void
nsWindow::SetTransparencyMode(nsTransparencyMode aMode)
{
    if (!mShell) {
        
        GtkWidget *topWidget = GetToplevelWidget();
        if (!topWidget)
            return;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return;

        topWindow->SetTransparencyMode(aMode);
        return;
    }
    bool isTransparent = aMode == eTransparencyTransparent;

    if (mIsTransparent == isTransparent)
        return;

    if (!isTransparent) {
        ClearTransparencyBitmap();
    } 
    

    mIsTransparent = isTransparent;

    
    
    CleanLayerManagerRecursive();
}

nsTransparencyMode
nsWindow::GetTransparencyMode()
{
    if (!mShell) {
        
        GtkWidget *topWidget = GetToplevelWidget();
        if (!topWidget) {
            return eTransparencyOpaque;
        }

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow) {
            return eTransparencyOpaque;
        }

        return topWindow->GetTransparencyMode();
    }

    return mIsTransparent ? eTransparencyTransparent : eTransparencyOpaque;
}

nsresult
nsWindow::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
    
    
    
    if (mWindowType == eWindowType_plugin_ipc_chrome) {
      return NS_OK;
    }

    for (uint32_t i = 0; i < aConfigurations.Length(); ++i) {
        const Configuration& configuration = aConfigurations[i];
        nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
        NS_ASSERTION(w->GetParent() == this,
                     "Configured widget is not a child");
        w->SetWindowClipRegion(configuration.mClipRegion, true);
        if (w->mBounds.Size() != configuration.mBounds.Size()) {
            w->Resize(configuration.mBounds.x, configuration.mBounds.y,
                      configuration.mBounds.width, configuration.mBounds.height,
                      true);
        } else if (w->mBounds.TopLeft() != configuration.mBounds.TopLeft()) {
            w->Move(configuration.mBounds.x, configuration.mBounds.y);
        } 
        w->SetWindowClipRegion(configuration.mClipRegion, false);
    }
    return NS_OK;
}

static pixman_box32
ToPixmanBox(const nsIntRect& aRect)
{
    pixman_box32_t result;
    result.x1 = aRect.x;
    result.y1 = aRect.y;
    result.x2 = aRect.XMost();
    result.y2 = aRect.YMost();
    return result;
}

static nsIntRect
ToIntRect(const pixman_box32& aBox)
{
    nsIntRect result;
    result.x = aBox.x1;
    result.y = aBox.y1;
    result.width = aBox.x2 - aBox.x1;
    result.height = aBox.y2 - aBox.y1;
    return result;
}

static void
InitRegion(pixman_region32* aRegion,
           const nsTArray<nsIntRect>& aRects)
{
    nsAutoTArray<pixman_box32,10> rects;
    rects.SetCapacity(aRects.Length());
    for (uint32_t i = 0; i < aRects.Length (); ++i) {
        if (!aRects[i].IsEmpty()) {
            rects.AppendElement(ToPixmanBox(aRects[i]));
        }
    }

    pixman_region32_init_rects(aRegion,
                               rects.Elements(), rects.Length());
}

static void
GetIntRects(pixman_region32& aRegion, nsTArray<nsIntRect>* aRects)
{
    int nRects;
    pixman_box32* boxes = pixman_region32_rectangles(&aRegion, &nRects);
    aRects->SetCapacity(aRects->Length() + nRects);
    for (int i = 0; i < nRects; ++i) {
        aRects->AppendElement(ToIntRect(boxes[i]));
    }
}

nsresult
nsWindow::SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                              bool aIntersectWithExisting)
{
    const nsTArray<nsIntRect>* newRects = &aRects;

    nsAutoTArray<nsIntRect,1> intersectRects;
    if (aIntersectWithExisting) {
        nsAutoTArray<nsIntRect,1> existingRects;
        GetWindowClipRegion(&existingRects);

        nsAutoRef<pixman_region32> existingRegion;
        InitRegion(&existingRegion, existingRects);
        nsAutoRef<pixman_region32> newRegion;
        InitRegion(&newRegion, aRects);
        nsAutoRef<pixman_region32> intersectRegion;
        pixman_region32_init(&intersectRegion);
        pixman_region32_intersect(&intersectRegion,
                                  &newRegion, &existingRegion);

        
        
        if (mClipRects &&
            pixman_region32_equal(&intersectRegion, &existingRegion)) {
            return NS_OK;
        }

        if (!pixman_region32_equal(&intersectRegion, &newRegion)) {
            GetIntRects(intersectRegion, &intersectRects);
            newRects = &intersectRects;
        }
    }

    if (IsWindowClipRegionEqual(*newRects))
        return NS_OK;

    StoreWindowClipRegion(*newRects);

    if (!mGdkWindow)
        return NS_OK;

#if (MOZ_WIDGET_GTK == 2)
    GdkRegion *region = gdk_region_new(); 
    for (uint32_t i = 0; i < newRects->Length(); ++i) {
        const nsIntRect& r = newRects->ElementAt(i);
        GdkRectangle rect = { r.x, r.y, r.width, r.height };
        gdk_region_union_with_rect(region, &rect);
    }

    gdk_window_shape_combine_region(mGdkWindow, region, 0, 0);
    gdk_region_destroy(region);
#else
    cairo_region_t *region = cairo_region_create();
    for (uint32_t i = 0; i < newRects->Length(); ++i) {
        const nsIntRect& r = newRects->ElementAt(i);
        cairo_rectangle_int_t rect = { r.x, r.y, r.width, r.height };
        cairo_region_union_rectangle(region, &rect);
    }

    gdk_window_shape_combine_region(mGdkWindow, region, 0, 0);
    cairo_region_destroy(region);
#endif

    return NS_OK;
}

void
nsWindow::ResizeTransparencyBitmap()
{
    if (!mTransparencyBitmap)
        return;

    if (mBounds.width == mTransparencyBitmapWidth &&
        mBounds.height == mTransparencyBitmapHeight)
        return;

    int32_t newRowBytes = GetBitmapStride(mBounds.width);
    int32_t newSize = newRowBytes * mBounds.height;
    gchar* newBits = new gchar[newSize];
    
    memset(newBits, 0, newSize);

    
    int32_t copyWidth = std::min(mBounds.width, mTransparencyBitmapWidth);
    int32_t copyHeight = std::min(mBounds.height, mTransparencyBitmapHeight);
    int32_t oldRowBytes = GetBitmapStride(mTransparencyBitmapWidth);
    int32_t copyBytes = GetBitmapStride(copyWidth);

    int32_t i;
    gchar* fromPtr = mTransparencyBitmap;
    gchar* toPtr = newBits;
    for (i = 0; i < copyHeight; i++) {
        memcpy(toPtr, fromPtr, copyBytes);
        fromPtr += oldRowBytes;
        toPtr += newRowBytes;
    }

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = newBits;
    mTransparencyBitmapWidth = mBounds.width;
    mTransparencyBitmapHeight = mBounds.height;
}

static bool
ChangedMaskBits(gchar* aMaskBits, int32_t aMaskWidth, int32_t aMaskHeight,
        const nsIntRect& aRect, uint8_t* aAlphas, int32_t aStride)
{
    int32_t x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
    int32_t maskBytesPerRow = GetBitmapStride(aMaskWidth);
    for (y = aRect.y; y < yMax; y++) {
        gchar* maskBytes = aMaskBits + y*maskBytesPerRow;
        uint8_t* alphas = aAlphas;
        for (x = aRect.x; x < xMax; x++) {
            bool newBit = *alphas > 0x7f;
            alphas++;

            gchar maskByte = maskBytes[x >> 3];
            bool maskBit = (maskByte & (1 << (x & 7))) != 0;

            if (maskBit != newBit) {
                return true;
            }
        }
        aAlphas += aStride;
    }

    return false;
}

static
void UpdateMaskBits(gchar* aMaskBits, int32_t aMaskWidth, int32_t aMaskHeight,
        const nsIntRect& aRect, uint8_t* aAlphas, int32_t aStride)
{
    int32_t x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
    int32_t maskBytesPerRow = GetBitmapStride(aMaskWidth);
    for (y = aRect.y; y < yMax; y++) {
        gchar* maskBytes = aMaskBits + y*maskBytesPerRow;
        uint8_t* alphas = aAlphas;
        for (x = aRect.x; x < xMax; x++) {
            bool newBit = *alphas > 0x7f;
            alphas++;

            gchar mask = 1 << (x & 7);
            gchar maskByte = maskBytes[x >> 3];
            
            maskBytes[x >> 3] = (maskByte & ~mask) | (-newBit & mask);
        }
        aAlphas += aStride;
    }
}

void
nsWindow::ApplyTransparencyBitmap()
{
#ifdef MOZ_X11
    
    
    
    GdkWindow *shellWindow = gtk_widget_get_window(mShell);
    Display* xDisplay = GDK_WINDOW_XDISPLAY(shellWindow);
    Window xDrawable = GDK_WINDOW_XID(shellWindow);
    Pixmap maskPixmap = XCreateBitmapFromData(xDisplay,
                                              xDrawable,
                                              mTransparencyBitmap,
                                              mTransparencyBitmapWidth,
                                              mTransparencyBitmapHeight);
    XShapeCombineMask(xDisplay, xDrawable,
                      ShapeBounding, 0, 0,
                      maskPixmap, ShapeSet);
    XFreePixmap(xDisplay, maskPixmap);
#else
#if (MOZ_WIDGET_GTK == 2)
    gtk_widget_reset_shapes(mShell);
    GdkBitmap* maskBitmap = gdk_bitmap_create_from_data(gtk_widget_get_window(mShell),
            mTransparencyBitmap,
            mTransparencyBitmapWidth, mTransparencyBitmapHeight);
    if (!maskBitmap)
        return;

    gtk_widget_shape_combine_mask(mShell, maskBitmap, 0, 0);
    g_object_unref(maskBitmap);
#else
    cairo_surface_t *maskBitmap;
    maskBitmap = cairo_image_surface_create_for_data((unsigned char*)mTransparencyBitmap, 
                                                     CAIRO_FORMAT_A1, 
                                                     mTransparencyBitmapWidth, 
                                                     mTransparencyBitmapHeight,
                                                     GetBitmapStride(mTransparencyBitmapWidth));
    if (!maskBitmap)
        return;

    cairo_region_t * maskRegion = gdk_cairo_region_create_from_surface(maskBitmap);
    gtk_widget_shape_combine_region(mShell, maskRegion);
    cairo_region_destroy(maskRegion);
    cairo_surface_destroy(maskBitmap);
#endif 
#endif 
}

void
nsWindow::ClearTransparencyBitmap()
{
    if (!mTransparencyBitmap)
        return;

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = nullptr;
    mTransparencyBitmapWidth = 0;
    mTransparencyBitmapHeight = 0;

    if (!mShell)
        return;

#ifdef MOZ_X11
    GdkWindow *window = gtk_widget_get_window(mShell);
    if (!window)
        return;

    Display* xDisplay = GDK_WINDOW_XDISPLAY(window);
    Window xWindow = gdk_x11_window_get_xid(window);

    XShapeCombineMask(xDisplay, xWindow, ShapeBounding, 0, 0, None, ShapeSet);
#endif
}

nsresult
nsWindow::UpdateTranslucentWindowAlphaInternal(const nsIntRect& aRect,
                                               uint8_t* aAlphas, int32_t aStride)
{
    if (!mShell) {
        
        GtkWidget *topWidget = GetToplevelWidget();
        if (!topWidget)
            return NS_ERROR_FAILURE;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return NS_ERROR_FAILURE;

        return topWindow->UpdateTranslucentWindowAlphaInternal(aRect, aAlphas, aStride);
    }

    NS_ASSERTION(mIsTransparent, "Window is not transparent");

    if (mTransparencyBitmap == nullptr) {
        int32_t size = GetBitmapStride(mBounds.width)*mBounds.height;
        mTransparencyBitmap = new gchar[size];
        memset(mTransparencyBitmap, 255, size);
        mTransparencyBitmapWidth = mBounds.width;
        mTransparencyBitmapHeight = mBounds.height;
    } else {
        ResizeTransparencyBitmap();
    }

    nsIntRect rect;
    rect.IntersectRect(aRect, nsIntRect(0, 0, mBounds.width, mBounds.height));

    if (!ChangedMaskBits(mTransparencyBitmap, mBounds.width, mBounds.height,
                         rect, aAlphas, aStride))
        
        
        return NS_OK;

    UpdateMaskBits(mTransparencyBitmap, mBounds.width, mBounds.height,
                   rect, aAlphas, aStride);

    if (!mNeedsShow) {
        ApplyTransparencyBitmap();
    }
    return NS_OK;
}

void
nsWindow::GrabPointer(guint32 aTime)
{
    LOG(("GrabPointer time=0x%08x retry=%d\n",
         (unsigned int)aTime, mRetryPointerGrab));

    mRetryPointerGrab = false;
    sRetryGrabTime = aTime;

    
    
    
    if (!mHasMappedToplevel || mIsFullyObscured) {
        LOG(("GrabPointer: window not visible\n"));
        mRetryPointerGrab = true;
        return;
    }

    if (!mGdkWindow)
        return;

    gint retval;
    retval = gdk_pointer_grab(mGdkWindow, TRUE,
                              (GdkEventMask)(GDK_BUTTON_PRESS_MASK |
                                             GDK_BUTTON_RELEASE_MASK |
                                             GDK_ENTER_NOTIFY_MASK |
                                             GDK_LEAVE_NOTIFY_MASK |
                                             GDK_POINTER_MOTION_MASK),
                              (GdkWindow *)nullptr, nullptr, aTime);

    if (retval == GDK_GRAB_NOT_VIEWABLE) {
        LOG(("GrabPointer: window not viewable; will retry\n"));
        mRetryPointerGrab = true;
    } else if (retval != GDK_GRAB_SUCCESS) {
        LOG(("GrabPointer: pointer grab failed: %i\n", retval));
        
        
        
        CheckForRollup(0, 0, false, true);
    }
}

void
nsWindow::ReleaseGrabs(void)
{
    LOG(("ReleaseGrabs\n"));

    mRetryPointerGrab = false;
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
}

GtkWidget *
nsWindow::GetToplevelWidget()
{
    if (mShell) {
        return mShell;
    }

    GtkWidget *widget = GetMozContainerWidget();
    if (!widget)
        return nullptr;

    return gtk_widget_get_toplevel(widget);
}

GtkWidget *
nsWindow::GetMozContainerWidget()
{
    if (!mGdkWindow)
        return nullptr;

    if (mContainer)
        return GTK_WIDGET(mContainer);

    GtkWidget *owningWidget =
        get_gtk_widget_for_gdk_window(mGdkWindow);
    return owningWidget;
}

nsWindow *
nsWindow::GetContainerWindow()
{
    GtkWidget *owningWidget = GetMozContainerWidget();
    if (!owningWidget)
        return nullptr;

    nsWindow *window = get_window_for_gtk_widget(owningWidget);
    NS_ASSERTION(window, "No nsWindow for container widget");
    return window;
}

void
nsWindow::SetUrgencyHint(GtkWidget *top_window, bool state)
{
    if (!top_window)
        return;
        
    gdk_window_set_urgency_hint(gtk_widget_get_window(top_window), state);
}

void *
nsWindow::SetupPluginPort(void)
{
    if (!mGdkWindow)
        return nullptr;

    if (gdk_window_is_destroyed(mGdkWindow) == TRUE)
        return nullptr;

    Window window = gdk_x11_window_get_xid(mGdkWindow);
    
    
    
    
#ifdef MOZ_X11
    XWindowAttributes xattrs;    
    Display *display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    XGetWindowAttributes(display, window, &xattrs);
    XSelectInput (display, window,
                  xattrs.your_event_mask |
                  SubstructureNotifyMask);

    gdk_window_add_filter(mGdkWindow, plugin_window_filter_func, this);

    XSync(display, False);
#endif 
    
    return (void *)window;
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

#ifdef MOZ_X11
void
nsWindow::SetNonXEmbedPluginFocus()
{
    if (gPluginFocusWindow == this || mPluginType!=PluginType_NONXEMBED) {
        return;
    }

    if (gPluginFocusWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gPluginFocusWindow;
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }

    LOGFOCUS(("nsWindow::SetNonXEmbedPluginFocus\n"));

    Window curFocusWindow;
    int focusState;
    
    GdkDisplay *gdkDisplay = gdk_window_get_display(mGdkWindow);
    XGetInputFocus(gdk_x11_display_get_xdisplay(gdkDisplay),
                   &curFocusWindow,
                   &focusState);

    LOGFOCUS(("\t curFocusWindow=%p\n", curFocusWindow));

    GdkWindow* toplevel = gdk_window_get_toplevel(mGdkWindow);
#if (MOZ_WIDGET_GTK == 2)
    GdkWindow *gdkfocuswin = gdk_window_lookup(curFocusWindow);
#else
    GdkWindow *gdkfocuswin = gdk_x11_window_lookup_for_display(gdkDisplay,
                                                               curFocusWindow);
#endif

    
    
    
    if (gdkfocuswin != toplevel) {
        return;
    }

    
    mOldFocusWindow = curFocusWindow;
    XRaiseWindow(GDK_WINDOW_XDISPLAY(mGdkWindow),
                 gdk_x11_window_get_xid(mGdkWindow));
    gdk_error_trap_push();
    XSetInputFocus(GDK_WINDOW_XDISPLAY(mGdkWindow),
                   gdk_x11_window_get_xid(mGdkWindow),
                   RevertToNone,
                   CurrentTime);
    gdk_flush();
#if (MOZ_WIDGET_GTK == 3)
    gdk_error_trap_pop_ignored();
#else
    gdk_error_trap_pop();
#endif
    gPluginFocusWindow = this;
    gdk_window_add_filter(nullptr, plugin_client_message_filter, this);

    LOGFOCUS(("nsWindow::SetNonXEmbedPluginFocus oldfocus=%p new=%p\n",
              mOldFocusWindow, gdk_x11_window_get_xid(mGdkWindow)));
}

void
nsWindow::LoseNonXEmbedPluginFocus()
{
    LOGFOCUS(("nsWindow::LoseNonXEmbedPluginFocus\n"));

    
    
    if (gPluginFocusWindow != this || mPluginType!=PluginType_NONXEMBED) {
        return;
    }

    Window curFocusWindow;
    int focusState;

    XGetInputFocus(GDK_WINDOW_XDISPLAY(mGdkWindow),
                   &curFocusWindow,
                   &focusState);

    
    
    
    
    if (!curFocusWindow ||
        curFocusWindow == gdk_x11_window_get_xid(mGdkWindow)) {

        gdk_error_trap_push();
        XRaiseWindow(GDK_WINDOW_XDISPLAY(mGdkWindow),
                     mOldFocusWindow);
        XSetInputFocus(GDK_WINDOW_XDISPLAY(mGdkWindow),
                       mOldFocusWindow,
                       RevertToParent,
                       CurrentTime);
        gdk_flush();
#if (MOZ_WIDGET_GTK == 3)
        gdk_error_trap_pop_ignored();
#else
        gdk_error_trap_pop();
#endif
    }
    gPluginFocusWindow = nullptr;
    mOldFocusWindow = 0;
    gdk_window_remove_filter(nullptr, plugin_client_message_filter, this);

    LOGFOCUS(("nsWindow::LoseNonXEmbedPluginFocus end\n"));
}
#endif 

gint
nsWindow::ConvertBorderStyles(nsBorderStyle aStyle)
{
    gint w = 0;

    if (aStyle == eBorderStyle_default)
        return -1;

    
    if (aStyle & eBorderStyle_all)
        w |= GDK_DECOR_ALL;
    if (aStyle & eBorderStyle_border)
        w |= GDK_DECOR_BORDER;
    if (aStyle & eBorderStyle_resizeh)
        w |= GDK_DECOR_RESIZEH;
    if (aStyle & eBorderStyle_title)
        w |= GDK_DECOR_TITLE;
    if (aStyle & eBorderStyle_menu)
        w |= GDK_DECOR_MENU;
    if (aStyle & eBorderStyle_minimize)
        w |= GDK_DECOR_MINIMIZE;
    if (aStyle & eBorderStyle_maximize)
        w |= GDK_DECOR_MAXIMIZE;

    return w;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(bool aFullScreen, nsIScreen* aTargetScreen)
{
    LOG(("nsWindow::MakeFullScreen [%p] aFullScreen %d\n",
         (void *)this, aFullScreen));

    if (aFullScreen) {
        if (mSizeMode != nsSizeMode_Fullscreen)
            mLastSizeMode = mSizeMode;

        mSizeMode = nsSizeMode_Fullscreen;
        gtk_window_fullscreen(GTK_WINDOW(mShell));
    }
    else {
        mSizeMode = mLastSizeMode;
        gtk_window_unfullscreen(GTK_WINDOW(mShell));
    }

    NS_ASSERTION(mLastSizeMode != nsSizeMode_Fullscreen,
                 "mLastSizeMode should never be fullscreen");
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::HideWindowChrome(bool aShouldHide)
{
    if (!mShell) {
        
        GtkWidget *topWidget = GetToplevelWidget();
        if (!topWidget)
            return NS_ERROR_FAILURE;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return NS_ERROR_FAILURE;

        return topWindow->HideWindowChrome(aShouldHide);
    }

    
    
    
    bool wasVisible = false;
    GdkWindow *shellWindow = gtk_widget_get_window(mShell);
    if (gdk_window_is_visible(shellWindow)) {
        gdk_window_hide(shellWindow);
        wasVisible = true;
    }

    gint wmd;
    if (aShouldHide)
        wmd = 0;
    else
        wmd = ConvertBorderStyles(mBorderStyle);

    if (wmd != -1)
      gdk_window_set_decorations(shellWindow, (GdkWMDecoration) wmd);

    if (wasVisible)
        gdk_window_show(shellWindow);

    
    
    
    
    
#ifdef MOZ_X11
    XSync(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()) , False);
#else
    gdk_flush ();
#endif 

    return NS_OK;
}

bool
nsWindow::CheckForRollup(gdouble aMouseX, gdouble aMouseY,
                         bool aIsWheel, bool aAlwaysRollup)
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
    GdkWindow *currentPopup =
        (GdkWindow *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);
    if (aAlwaysRollup || !is_mouse_in_window(currentPopup, aMouseX, aMouseY)) {
        bool rollup = true;
        if (aIsWheel) {
            rollup = rollupListener->ShouldRollupOnMouseWheelEvent();
            retVal = rollupListener->ShouldConsumeOnMouseWheelEvent();
        }
        
        
        
        uint32_t popupsToRollup = UINT32_MAX;
        if (!aAlwaysRollup) {
            nsAutoTArray<nsIWidget*, 5> widgetChain;
            uint32_t sameTypeCount = rollupListener->GetSubmenuWidgetChain(&widgetChain);
            for (uint32_t i=0; i<widgetChain.Length(); ++i) {
                nsIWidget* widget = widgetChain[i];
                GdkWindow* currWindow =
                    (GdkWindow*) widget->GetNativeData(NS_NATIVE_WINDOW);
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

        
        bool usePoint = !aIsWheel && !aAlwaysRollup;
        nsIntPoint point(aMouseX, aMouseY);
        if (rollup && rollupListener->Rollup(popupsToRollup, true, usePoint ? &point : nullptr, nullptr)) {
            retVal = true;
        }
    }
    return retVal;
}


bool
nsWindow::DragInProgress(void)
{
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);

    if (!dragService)
        return false;

    nsCOMPtr<nsIDragSession> currentDragSession;
    dragService->GetCurrentSession(getter_AddRefs(currentDragSession));

    return currentDragSession != nullptr;
}

static bool
is_mouse_in_window (GdkWindow* aWindow, gdouble aMouseX, gdouble aMouseY)
{
    gint x = 0;
    gint y = 0;
    gint w, h;

    gint offsetX = 0;
    gint offsetY = 0;

    GdkWindow *window = aWindow;

    while (window) {
        gint tmpX = 0;
        gint tmpY = 0;

        gdk_window_get_position(window, &tmpX, &tmpY);
        GtkWidget *widget = get_gtk_widget_for_gdk_window(window);

        
        
        if (GTK_IS_WINDOW(widget)) {
            x = tmpX + offsetX;
            y = tmpY + offsetY;
            break;
        }

        offsetX += tmpX;
        offsetY += tmpY;
        window = gdk_window_get_parent(window);
    }

#if (MOZ_WIDGET_GTK == 2)
    gdk_drawable_get_size(aWindow, &w, &h);
#else
    w = gdk_window_get_width(aWindow);
    h = gdk_window_get_height(aWindow);
#endif

    if (aMouseX > x && aMouseX < x + w &&
        aMouseY > y && aMouseY < y + h)
        return true;

    return false;
}

static nsWindow *
get_window_for_gtk_widget(GtkWidget *widget)
{
    gpointer user_data = g_object_get_data(G_OBJECT(widget), "nsWindow");

    return static_cast<nsWindow *>(user_data);
}

static nsWindow *
get_window_for_gdk_window(GdkWindow *window)
{
    gpointer user_data = g_object_get_data(G_OBJECT(window), "nsWindow");

    return static_cast<nsWindow *>(user_data);
}

static GtkWidget *
get_gtk_widget_for_gdk_window(GdkWindow *window)
{
    gpointer user_data = nullptr;
    gdk_window_get_user_data(window, &user_data);

    return GTK_WIDGET(user_data);
}

static GdkCursor *
get_gtk_cursor(nsCursor aCursor)
{
    GdkCursor *gdkcursor = nullptr;
    uint8_t newType = 0xff;

    if ((gdkcursor = gCursorCache[aCursor])) {
        return gdkcursor;
    }
    
    GdkDisplay *defaultDisplay = gdk_display_get_default();

    
    
    
    switch (aCursor) {
    case eCursor_standard:
        gdkcursor = gdk_cursor_new(GDK_LEFT_PTR);
        break;
    case eCursor_wait:
        gdkcursor = gdk_cursor_new(GDK_WATCH);
        break;
    case eCursor_select:
        gdkcursor = gdk_cursor_new(GDK_XTERM);
        break;
    case eCursor_hyperlink:
        gdkcursor = gdk_cursor_new(GDK_HAND2);
        break;
    case eCursor_n_resize:
        gdkcursor = gdk_cursor_new(GDK_TOP_SIDE);
        break;
    case eCursor_s_resize:
        gdkcursor = gdk_cursor_new(GDK_BOTTOM_SIDE);
        break;
    case eCursor_w_resize:
        gdkcursor = gdk_cursor_new(GDK_LEFT_SIDE);
        break;
    case eCursor_e_resize:
        gdkcursor = gdk_cursor_new(GDK_RIGHT_SIDE);
        break;
    case eCursor_nw_resize:
        gdkcursor = gdk_cursor_new(GDK_TOP_LEFT_CORNER);
        break;
    case eCursor_se_resize:
        gdkcursor = gdk_cursor_new(GDK_BOTTOM_RIGHT_CORNER);
        break;
    case eCursor_ne_resize:
        gdkcursor = gdk_cursor_new(GDK_TOP_RIGHT_CORNER);
        break;
    case eCursor_sw_resize:
        gdkcursor = gdk_cursor_new(GDK_BOTTOM_LEFT_CORNER);
        break;
    case eCursor_crosshair:
        gdkcursor = gdk_cursor_new(GDK_CROSSHAIR);
        break;
    case eCursor_move:
        gdkcursor = gdk_cursor_new(GDK_FLEUR);
        break;
    case eCursor_help:
        gdkcursor = gdk_cursor_new(GDK_QUESTION_ARROW);
        break;
    case eCursor_copy: 
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "copy");
        if (!gdkcursor)
            newType = MOZ_CURSOR_COPY;
        break;
    case eCursor_alias:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "alias");
        if (!gdkcursor)
            newType = MOZ_CURSOR_ALIAS;
        break;
    case eCursor_context_menu:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "context-menu");
        if (!gdkcursor)
            newType = MOZ_CURSOR_CONTEXT_MENU;
        break;
    case eCursor_cell:
        gdkcursor = gdk_cursor_new(GDK_PLUS);
        break;
    
    case eCursor_grab:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "openhand");
        if (!gdkcursor)
            newType = MOZ_CURSOR_HAND_GRAB;
        break;
    case eCursor_grabbing:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "closedhand");
        if (!gdkcursor)
            gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "grabbing");
        if (!gdkcursor)
            newType = MOZ_CURSOR_HAND_GRABBING;
        break;
    case eCursor_spinning:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "progress");
        if (!gdkcursor)
            newType = MOZ_CURSOR_SPINNING;
        break;
    case eCursor_zoom_in:
        newType = MOZ_CURSOR_ZOOM_IN;
        break;
    case eCursor_zoom_out:
        newType = MOZ_CURSOR_ZOOM_OUT;
        break;
    case eCursor_not_allowed:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "not-allowed");
        if (!gdkcursor) 
            gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "crossed_circle");
        if (!gdkcursor)
            newType = MOZ_CURSOR_NOT_ALLOWED;
        break;
    case eCursor_no_drop:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "no-drop");
        if (!gdkcursor) 
            gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "forbidden");
        if (!gdkcursor)
            gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "circle");
        if (!gdkcursor)
            newType = MOZ_CURSOR_NOT_ALLOWED;
        break;
    case eCursor_vertical_text:
        newType = MOZ_CURSOR_VERTICAL_TEXT;
        break;
    case eCursor_all_scroll:
        gdkcursor = gdk_cursor_new(GDK_FLEUR);
        break;
    case eCursor_nesw_resize:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "size_bdiag");
        if (!gdkcursor)
            newType = MOZ_CURSOR_NESW_RESIZE;
        break;
    case eCursor_nwse_resize:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "size_fdiag");
        if (!gdkcursor)
            newType = MOZ_CURSOR_NWSE_RESIZE;
        break;
    case eCursor_ns_resize:
        gdkcursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
        break;
    case eCursor_ew_resize:
        gdkcursor = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
        break;
    
    case eCursor_row_resize:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "split_v");
        if (!gdkcursor)
            gdkcursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
        break;
    case eCursor_col_resize:
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, "split_h");
        if (!gdkcursor)
            gdkcursor = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
        break;
    case eCursor_none:
        newType = MOZ_CURSOR_NONE;
        break;
    default:
        NS_ASSERTION(aCursor, "Invalid cursor type");
        gdkcursor = gdk_cursor_new(GDK_LEFT_PTR);
        break;
    }

    
    
    
    
    if (newType != 0xFF && GtkCursors[newType].hash) {
        gdkcursor = gdk_cursor_new_from_name(defaultDisplay, GtkCursors[newType].hash);
    }

    
    if (newType != 0xff && !gdkcursor) {
        GdkPixbuf * cursor_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 32, 32);
        if (!cursor_pixbuf)
            return nullptr;
      
        guchar *data = gdk_pixbuf_get_pixels(cursor_pixbuf);
        
        
        
        
        const unsigned char *bits = GtkCursors[newType].bits;
        const unsigned char *mask_bits = GtkCursors[newType].mask_bits;
        
        for (int i = 0; i < 128; i++) {
            char bit = *bits++;
            char mask = *mask_bits++;
            for (int j = 0; j < 8; j++) {
                unsigned char pix = ~(((bit >> j) & 0x01) * 0xff);
                *data++ = pix;
                *data++ = pix;
                *data++ = pix;
                *data++ = (((mask >> j) & 0x01) * 0xff);
            }
        }
      
        gdkcursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), cursor_pixbuf,
                                               GtkCursors[newType].hot_x,
                                               GtkCursors[newType].hot_y);
        
        g_object_unref(cursor_pixbuf);
    }

    gCursorCache[aCursor] = gdkcursor;

    return gdkcursor;
}



#if (MOZ_WIDGET_GTK == 2)
static gboolean
expose_event_cb(GtkWidget *widget, GdkEventExpose *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return FALSE;

    window->OnExposeEvent(event);
    return FALSE;
}
#else
void
draw_window_of_widget(GtkWidget *widget, GdkWindow *aWindow, cairo_t *cr)
{
    if (gtk_cairo_should_draw_window(cr, aWindow)) {
        nsRefPtr<nsWindow> window = get_window_for_gdk_window(aWindow);
        if (!window) {
            NS_WARNING("Cannot get nsWindow from GtkWidget");
        }
        else {      
            cairo_save(cr);      
            gtk_cairo_transform_to_window(cr, widget, aWindow);  
            
            
            window->OnExposeEvent(cr);
            cairo_restore(cr);
        }
    }
    
    GList *children = gdk_window_get_children(aWindow);
    GList *child = children;
    while (child) {
        GdkWindow *window = GDK_WINDOW(child->data);
        gpointer windowWidget;
        gdk_window_get_user_data(window, &windowWidget);
        if (windowWidget == widget) {
            draw_window_of_widget(widget, window, cr);
        }
        child = g_list_next(child);
    }  
    g_list_free(children);
}


gboolean
expose_event_cb(GtkWidget *widget, cairo_t *cr)
{
    draw_window_of_widget(widget, gtk_widget_get_window(widget), cr);
    return FALSE;
}
#endif 

static gboolean
configure_event_cb(GtkWidget *widget,
                   GdkEventConfigure *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    return window->OnConfigureEvent(widget, event);
}

static void
container_unrealize_cb (GtkWidget *widget)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return;

    window->OnContainerUnrealize();
}

static void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return;

    window->OnSizeAllocate(allocation);
}

static gboolean
delete_event_cb(GtkWidget *widget, GdkEventAny *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnDeleteEvent();

    return TRUE;
}

static gboolean
enter_notify_event_cb(GtkWidget *widget,
                      GdkEventCrossing *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

    window->OnEnterNotifyEvent(event);

    return TRUE;
}

static gboolean
leave_notify_event_cb(GtkWidget *widget,
                      GdkEventCrossing *event)
{
    if (is_parent_grab_leave(event)) {
        return TRUE;
    }

    
    
    gint x = gint(event->x_root);
    gint y = gint(event->y_root);
    GdkDisplay* display = gtk_widget_get_display(widget);
    GdkWindow* winAtPt = gdk_display_get_window_at_pointer(display, &x, &y);
    if (winAtPt == event->window) {
        return TRUE;
    }

    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

    window->OnLeaveNotifyEvent(event);

    return TRUE;
}

static nsWindow*
GetFirstNSWindowForGDKWindow(GdkWindow *aGdkWindow)
{
    nsWindow* window;
    while (!(window = get_window_for_gdk_window(aGdkWindow))) {
        
        
        
        aGdkWindow = gdk_window_get_parent(aGdkWindow);
        if (!aGdkWindow) {
            window = nullptr;
            break;
        }
    }
    return window;
}

static gboolean
motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event)
{
    UpdateLastInputEventTime(event);

    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnMotionNotifyEvent(event);

    return TRUE;
}

static gboolean
button_press_event_cb(GtkWidget *widget, GdkEventButton *event)
{
    UpdateLastInputEventTime(event);

    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnButtonPressEvent(event);

    return TRUE;
}

static gboolean
button_release_event_cb(GtkWidget *widget, GdkEventButton *event)
{
    UpdateLastInputEventTime(event);

    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnButtonReleaseEvent(event);

    return TRUE;
}

static gboolean
focus_in_event_cb(GtkWidget *widget, GdkEventFocus *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnContainerFocusInEvent(event);

    return FALSE;
}

static gboolean
focus_out_event_cb(GtkWidget *widget, GdkEventFocus *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnContainerFocusOutEvent(event);

    return FALSE;
}

#ifdef MOZ_X11


















static GdkFilterReturn
popup_take_focus_filter(GdkXEvent *gdk_xevent,
                        GdkEvent *event,
                        gpointer data)
{
    XEvent* xevent = static_cast<XEvent*>(gdk_xevent);
    if (xevent->type != ClientMessage)
        return GDK_FILTER_CONTINUE;

    XClientMessageEvent& xclient = xevent->xclient;
    if (xclient.message_type != gdk_x11_get_xatom_by_name("WM_PROTOCOLS"))
        return GDK_FILTER_CONTINUE;

    Atom atom = xclient.data.l[0];
    if (atom != gdk_x11_get_xatom_by_name("WM_TAKE_FOCUS"))
        return GDK_FILTER_CONTINUE;

    guint32 timestamp = xclient.data.l[1];

    GtkWidget* widget = get_gtk_widget_for_gdk_window(event->any.window);
    if (!widget)
        return GDK_FILTER_CONTINUE;

    GtkWindow* parent = gtk_window_get_transient_for(GTK_WINDOW(widget));
    if (!parent)
        return GDK_FILTER_CONTINUE;

    if (gtk_window_is_active(parent))
        return GDK_FILTER_REMOVE; 

    GdkWindow* parent_window = gtk_widget_get_window(GTK_WIDGET(parent));
    if (!parent_window)
        return GDK_FILTER_CONTINUE;

    
    gdk_window_show_unraised(parent_window);

    
    
    
    gdk_window_focus(parent_window, timestamp);
    return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
plugin_window_filter_func(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
    GdkWindow  *plugin_window;
    XEvent     *xevent;
    Window      xeventWindow;

    nsRefPtr<nsWindow> nswindow = (nsWindow*)data;
    GdkFilterReturn return_val;

    xevent = (XEvent *)gdk_xevent;
    return_val = GDK_FILTER_CONTINUE;

    switch (xevent->type)
    {
        case CreateNotify:
        case ReparentNotify:
            if (xevent->type==CreateNotify) {
                xeventWindow = xevent->xcreatewindow.window;
            }
            else {
                if (xevent->xreparent.event != xevent->xreparent.parent)
                    break;
                xeventWindow = xevent->xreparent.window;
            }
#if (MOZ_WIDGET_GTK == 2)
            plugin_window = gdk_window_lookup(xeventWindow);
#else
            plugin_window = gdk_x11_window_lookup_for_display(
                                  gdk_x11_lookup_xdisplay(xevent->xcreatewindow.display), xeventWindow);
#endif        
            if (plugin_window) {
                GtkWidget *widget =
                    get_gtk_widget_for_gdk_window(plugin_window);


#if (MOZ_WIDGET_GTK == 2)
                if (GTK_IS_XTBIN(widget)) {
                    nswindow->SetPluginType(nsWindow::PluginType_NONXEMBED);
                    break;
                }
                else 
#endif
                if(GTK_IS_SOCKET(widget)) {
                    if (!g_object_get_data(G_OBJECT(widget), "enable-xt-focus")) {
                        nswindow->SetPluginType(nsWindow::PluginType_XEMBED);
                        break;
                    }
                }
            }
            nswindow->SetPluginType(nsWindow::PluginType_NONXEMBED);
            return_val = GDK_FILTER_REMOVE;
            break;
        case EnterNotify:
            nswindow->SetNonXEmbedPluginFocus();
            break;
        case DestroyNotify:
            gdk_window_remove_filter
                ((GdkWindow*)(nswindow->GetNativeData(NS_NATIVE_WINDOW)),
                 plugin_window_filter_func,
                 nswindow);
            
            
            nswindow->LoseNonXEmbedPluginFocus();
            break;
        default:
            break;
    }
    return return_val;
}

static GdkFilterReturn
plugin_client_message_filter(GdkXEvent *gdk_xevent,
                             GdkEvent *event,
                             gpointer data)
{
    XEvent    *xevent;
    xevent = (XEvent *)gdk_xevent;

    GdkFilterReturn return_val;
    return_val = GDK_FILTER_CONTINUE;

    if (!gPluginFocusWindow || xevent->type!=ClientMessage) {
        return return_val;
    }

    
    
    
    
    if (gdk_x11_get_xatom_by_name("WM_PROTOCOLS")
            != xevent->xclient.message_type) {
        return return_val;
    }

    if ((Atom) xevent->xclient.data.l[0] ==
            gdk_x11_get_xatom_by_name("WM_TAKE_FOCUS")) {
        
        return_val = GDK_FILTER_REMOVE;
    }

    return return_val;
}
#endif 

static gboolean
key_press_event_cb(GtkWidget *widget, GdkEventKey *event)
{
    LOG(("key_press_event_cb\n"));

    UpdateLastInputEventTime(event);

    
    nsWindow *window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    nsRefPtr<nsWindow> focusWindow = gFocusWindow ? gFocusWindow : window;

#ifdef MOZ_X11
    
    
    
    
    
    
#define NS_GDKEVENT_MATCH_MASK 0x1FFF /* GDK_SHIFT_MASK .. GDK_BUTTON5_MASK */
    GdkDisplay* gdkDisplay = gtk_widget_get_display(widget);
    if (GDK_IS_X11_DISPLAY(gdkDisplay)) {
        Display* dpy = GDK_DISPLAY_XDISPLAY(gdkDisplay);
        while (XPending(dpy)) {
            XEvent next_event;
            XPeekEvent(dpy, &next_event);
            GdkWindow* nextGdkWindow =
                gdk_x11_window_lookup_for_display(gdkDisplay, next_event.xany.window);
            if (nextGdkWindow != event->window ||
                next_event.type != KeyPress ||
                next_event.xkey.keycode != event->hardware_keycode ||
                next_event.xkey.state != (event->state & NS_GDKEVENT_MATCH_MASK)) {
                break;
            }
            XNextEvent(dpy, &next_event);
            event->time = next_event.xkey.time;
        }
    }
#endif

    return focusWindow->OnKeyPressEvent(event);
}

static gboolean
key_release_event_cb(GtkWidget *widget, GdkEventKey *event)
{
    LOG(("key_release_event_cb\n"));

    UpdateLastInputEventTime(event);

    
    nsWindow *window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    nsRefPtr<nsWindow> focusWindow = gFocusWindow ? gFocusWindow : window;

    return focusWindow->OnKeyReleaseEvent(event);
}

static gboolean
scroll_event_cb(GtkWidget *widget, GdkEventScroll *event)
{
    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnScrollEvent(event);

    return TRUE;
}

static gboolean
visibility_notify_event_cb (GtkWidget *widget, GdkEventVisibility *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return FALSE;

    window->OnVisibilityNotifyEvent(event);

    return TRUE;
}

static void
hierarchy_changed_cb (GtkWidget *widget,
                      GtkWidget *previous_toplevel)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel(widget);
    GdkWindowState old_window_state = GDK_WINDOW_STATE_WITHDRAWN;
    GdkEventWindowState event;

    event.new_window_state = GDK_WINDOW_STATE_WITHDRAWN;

    if (GTK_IS_WINDOW(previous_toplevel)) {
        g_signal_handlers_disconnect_by_func(previous_toplevel,
                                             FuncToGpointer(window_state_event_cb),
                                             widget);
        GdkWindow *win = gtk_widget_get_window(previous_toplevel);
        if (win) {
            old_window_state = gdk_window_get_state(win);
        }
    }

    if (GTK_IS_WINDOW(toplevel)) {
        g_signal_connect_swapped(toplevel, "window-state-event",
                                 G_CALLBACK(window_state_event_cb), widget);
        GdkWindow *win = gtk_widget_get_window(toplevel);
        if (win) {
            event.new_window_state = gdk_window_get_state(win);
        }
    }

    event.changed_mask = static_cast<GdkWindowState>
        (old_window_state ^ event.new_window_state);

    if (event.changed_mask) {
        event.type = GDK_WINDOW_STATE;
        event.window = nullptr;
        event.send_event = TRUE;
        window_state_event_cb(widget, &event);
    }
}

static gboolean
window_state_event_cb (GtkWidget *widget, GdkEventWindowState *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnWindowStateEvent(widget, event);

    return FALSE;
}

static void
theme_changed_cb (GtkSettings *settings, GParamSpec *pspec, nsWindow *data)
{
    nsRefPtr<nsWindow> window = data;
    window->ThemeChanged();
}




void
nsWindow::InitDragEvent(WidgetDragEvent &aEvent)
{
    
    guint modifierState = KeymapWrapper::GetCurrentModifierState();
    KeymapWrapper::InitInputEvent(aEvent, modifierState);
}

static gboolean
drag_motion_event_cb(GtkWidget *aWidget,
                     GdkDragContext *aDragContext,
                     gint aX,
                     gint aY,
                     guint aTime,
                     gpointer aData)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(aWidget);
    if (!window)
        return FALSE;

    
    nscoord retx = 0;
    nscoord rety = 0;

    GdkWindow *innerWindow =
        get_inner_gdk_window(gtk_widget_get_window(aWidget), aX, aY,
                             &retx, &rety);
    nsRefPtr<nsWindow> innerMostWindow = get_window_for_gdk_window(innerWindow);

    if (!innerMostWindow) {
        innerMostWindow = window;
    }

    LOGDRAG(("nsWindow drag-motion signal for %p\n", (void*)innerMostWindow));

    return nsDragService::GetInstance()->
        ScheduleMotionEvent(innerMostWindow, aDragContext,
                            nsIntPoint(retx, rety), aTime);
}

static void
drag_leave_event_cb(GtkWidget *aWidget,
                    GdkDragContext *aDragContext,
                    guint aTime,
                    gpointer aData)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(aWidget);
    if (!window)
        return;

    nsDragService *dragService = nsDragService::GetInstance();

    nsWindow *mostRecentDragWindow = dragService->GetMostRecentDestWindow();
    if (!mostRecentDragWindow) {
        
        
        
        
        
        return;
    }

    GtkWidget *mozContainer = mostRecentDragWindow->GetMozContainerWidget();
    if (aWidget != mozContainer)
    {
        
        
        
        return;
    }

    LOGDRAG(("nsWindow drag-leave signal for %p\n",
             (void*)mostRecentDragWindow));

    dragService->ScheduleLeaveEvent();
}


static gboolean
drag_drop_event_cb(GtkWidget *aWidget,
                   GdkDragContext *aDragContext,
                   gint aX,
                   gint aY,
                   guint aTime,
                   gpointer aData)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(aWidget);
    if (!window)
        return FALSE;

    
    nscoord retx = 0;
    nscoord rety = 0;

    GdkWindow *innerWindow =
        get_inner_gdk_window(gtk_widget_get_window(aWidget), aX, aY,
                             &retx, &rety);
    nsRefPtr<nsWindow> innerMostWindow = get_window_for_gdk_window(innerWindow);

    if (!innerMostWindow) {
        innerMostWindow = window;
    }

    LOGDRAG(("nsWindow drag-drop signal for %p\n", (void*)innerMostWindow));

    return nsDragService::GetInstance()->
        ScheduleDropEvent(innerMostWindow, aDragContext,
                          nsIntPoint(retx, rety), aTime);
}

static void
drag_data_received_event_cb(GtkWidget *aWidget,
                            GdkDragContext *aDragContext,
                            gint aX,
                            gint aY,
                            GtkSelectionData  *aSelectionData,
                            guint aInfo,
                            guint aTime,
                            gpointer aData)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(aWidget);
    if (!window)
        return;

    window->OnDragDataReceivedEvent(aWidget,
                                    aDragContext,
                                    aX, aY,
                                    aSelectionData,
                                    aInfo, aTime, aData);
}

static nsresult
initialize_prefs(void)
{
    gRaiseWindows =
        Preferences::GetBool("mozilla.widget.raise-on-setfocus", true);

    return NS_OK;
}

static GdkWindow *
get_inner_gdk_window (GdkWindow *aWindow,
                      gint x, gint y,
                      gint *retx, gint *rety)
{
    gint cx, cy, cw, ch;
    GList *children = gdk_window_peek_children(aWindow);
    for (GList *child = g_list_last(children);
         child;
         child = g_list_previous(child)) {
        GdkWindow *childWindow = (GdkWindow *) child->data;
        if (get_window_for_gdk_window(childWindow)) {
#if (MOZ_WIDGET_GTK == 2)
            gdk_window_get_geometry(childWindow, &cx, &cy, &cw, &ch, nullptr);
#else
            gdk_window_get_geometry(childWindow, &cx, &cy, &cw, &ch);
#endif
            if ((cx < x) && (x < (cx + cw)) &&
                (cy < y) && (y < (cy + ch)) &&
                gdk_window_is_visible(childWindow)) {
                return get_inner_gdk_window(childWindow,
                                            x - cx, y - cy,
                                            retx, rety);
            }
        }
    }
    *retx = x;
    *rety = y;
    return aWindow;
}

static inline bool
is_context_menu_key(const WidgetKeyboardEvent& aKeyEvent)
{
    return ((aKeyEvent.keyCode == NS_VK_F10 && aKeyEvent.IsShift() &&
             !aKeyEvent.IsControl() && !aKeyEvent.IsMeta() &&
             !aKeyEvent.IsAlt()) ||
            (aKeyEvent.keyCode == NS_VK_CONTEXT_MENU && !aKeyEvent.IsShift() &&
             !aKeyEvent.IsControl() && !aKeyEvent.IsMeta() &&
             !aKeyEvent.IsAlt()));
}

static int
is_parent_ungrab_enter(GdkEventCrossing *aEvent)
{
    return (GDK_CROSSING_UNGRAB == aEvent->mode) &&
        ((GDK_NOTIFY_ANCESTOR == aEvent->detail) ||
         (GDK_NOTIFY_VIRTUAL == aEvent->detail));

}

static int
is_parent_grab_leave(GdkEventCrossing *aEvent)
{
    return (GDK_CROSSING_GRAB == aEvent->mode) &&
        ((GDK_NOTIFY_ANCESTOR == aEvent->detail) ||
            (GDK_NOTIFY_VIRTUAL == aEvent->detail));
}

#ifdef ACCESSIBILITY
void
nsWindow::CreateRootAccessible()
{
    if (mIsTopLevel && !mRootAccessible) {
        LOG(("nsWindow:: Create Toplevel Accessibility\n"));
        mRootAccessible = GetRootAccessible();
    }
}

void
nsWindow::DispatchEventToRootAccessible(uint32_t aEventType)
{
    if (!a11y::ShouldA11yBeEnabled()) {
        return;
    }

    nsCOMPtr<nsIAccessibilityService> accService =
        services::GetAccessibilityService();
    if (!accService) {
        return;
    }

    
    a11y::Accessible* acc = GetRootAccessible();
    if (acc) {
        accService->FireAccessibleEvent(aEventType, acc);
    }
}

void
nsWindow::DispatchActivateEventAccessible(void)
{
    DispatchEventToRootAccessible(nsIAccessibleEvent::EVENT_WINDOW_ACTIVATE);
}

void
nsWindow::DispatchDeactivateEventAccessible(void)
{
    DispatchEventToRootAccessible(nsIAccessibleEvent::EVENT_WINDOW_DEACTIVATE);
}

void
nsWindow::DispatchMaximizeEventAccessible(void)
{
    DispatchEventToRootAccessible(nsIAccessibleEvent::EVENT_WINDOW_MAXIMIZE);
}

void
nsWindow::DispatchMinimizeEventAccessible(void)
{
    DispatchEventToRootAccessible(nsIAccessibleEvent::EVENT_WINDOW_MINIMIZE);
}

void
nsWindow::DispatchRestoreEventAccessible(void)
{
    DispatchEventToRootAccessible(nsIAccessibleEvent::EVENT_WINDOW_RESTORE);
}

#endif 



nsChildWindow::nsChildWindow()
{
}

nsChildWindow::~nsChildWindow()
{
}

nsresult
nsWindow::NotifyIMEInternal(const IMENotification& aIMENotification)
{
    if (MOZ_UNLIKELY(!mIMModule)) {
        return NS_ERROR_NOT_AVAILABLE;
    }
    switch (aIMENotification.mMessage) {
        case REQUEST_TO_COMMIT_COMPOSITION:
        case REQUEST_TO_CANCEL_COMPOSITION:
            return mIMModule->EndIMEComposition(this);
        case NOTIFY_IME_OF_FOCUS:
            mIMModule->OnFocusChangeInGecko(true);
            return NS_OK;
        case NOTIFY_IME_OF_BLUR:
            mIMModule->OnFocusChangeInGecko(false);
            return NS_OK;
        case NOTIFY_IME_OF_COMPOSITION_UPDATE:
            mIMModule->OnUpdateComposition();
            return NS_OK;
        case NOTIFY_IME_OF_SELECTION_CHANGE:
            mIMModule->OnSelectionChange(this);
            return NS_OK;
        default:
            return NS_ERROR_NOT_IMPLEMENTED;
    }
}

NS_IMETHODIMP_(void)
nsWindow::SetInputContext(const InputContext& aContext,
                          const InputContextAction& aAction)
{
    if (!mIMModule) {
        return;
    }
    mIMModule->SetInputContext(this, &aContext, &aAction);
}

NS_IMETHODIMP_(InputContext)
nsWindow::GetInputContext()
{
  InputContext context;
  if (!mIMModule) {
      context.mIMEState.mEnabled = IMEState::DISABLED;
      context.mIMEState.mOpen = IMEState::OPEN_STATE_NOT_SUPPORTED;
      
      
      
      context.mNativeIMEContext = this;
  } else {
      context = mIMModule->GetInputContext();
      context.mNativeIMEContext = mIMModule;
  }
  return context;
}

nsIMEUpdatePreference
nsWindow::GetIMEUpdatePreference()
{
    nsIMEUpdatePreference updatePreference(
        nsIMEUpdatePreference::NOTIFY_SELECTION_CHANGE);
    
    
    
    updatePreference.DontNotifyChangesCausedByComposition();
    return updatePreference;
}

bool
nsWindow::ExecuteNativeKeyBindingRemapped(NativeKeyBindingsType aType,
                                          const WidgetKeyboardEvent& aEvent,
                                          DoCommandCallback aCallback,
                                          void* aCallbackData,
                                          uint32_t aGeckoKeyCode,
                                          uint32_t aNativeKeyCode)
{
    WidgetKeyboardEvent modifiedEvent(aEvent);
    modifiedEvent.keyCode = aGeckoKeyCode;
    static_cast<GdkEventKey*>(modifiedEvent.mNativeKeyEvent)->keyval =
        aNativeKeyCode;

    NativeKeyBindings* keyBindings = NativeKeyBindings::GetInstance(aType);
    return keyBindings->Execute(modifiedEvent, aCallback, aCallbackData);
}

NS_IMETHODIMP_(bool)
nsWindow::ExecuteNativeKeyBinding(NativeKeyBindingsType aType,
                                  const WidgetKeyboardEvent& aEvent,
                                  DoCommandCallback aCallback,
                                  void* aCallbackData)
{
    if (aEvent.keyCode >= nsIDOMKeyEvent::DOM_VK_LEFT &&
        aEvent.keyCode <= nsIDOMKeyEvent::DOM_VK_DOWN) {

        
        
        WidgetQueryContentEvent query(true, NS_QUERY_SELECTED_TEXT, this);
        nsEventStatus status;
        DispatchEvent(&query, status);

        if (query.mSucceeded && query.mReply.mWritingMode.IsVertical()) {
            uint32_t geckoCode = 0;
            uint32_t gdkCode = 0;
            switch (aEvent.keyCode) {
            case nsIDOMKeyEvent::DOM_VK_LEFT:
                if (query.mReply.mWritingMode.IsVerticalLR()) {
                    geckoCode = nsIDOMKeyEvent::DOM_VK_UP;
                    gdkCode = GDK_Up;
                } else {
                    geckoCode = nsIDOMKeyEvent::DOM_VK_DOWN;
                    gdkCode = GDK_Down;
                }
                break;

            case nsIDOMKeyEvent::DOM_VK_RIGHT:
                if (query.mReply.mWritingMode.IsVerticalLR()) {
                    geckoCode = nsIDOMKeyEvent::DOM_VK_DOWN;
                    gdkCode = GDK_Down;
                } else {
                    geckoCode = nsIDOMKeyEvent::DOM_VK_UP;
                    gdkCode = GDK_Up;
                }
                break;

            case nsIDOMKeyEvent::DOM_VK_UP:
                geckoCode = nsIDOMKeyEvent::DOM_VK_LEFT;
                gdkCode = GDK_Left;
                break;

            case nsIDOMKeyEvent::DOM_VK_DOWN:
                geckoCode = nsIDOMKeyEvent::DOM_VK_RIGHT;
                gdkCode = GDK_Right;
                break;
            }

            return ExecuteNativeKeyBindingRemapped(aType, aEvent, aCallback,
                                                   aCallbackData,
                                                   geckoCode, gdkCode);
        }
    }

    NativeKeyBindings* keyBindings = NativeKeyBindings::GetInstance(aType);
    return keyBindings->Execute(aEvent, aCallback, aCallbackData);
}

NS_IMETHODIMP
nsWindow::GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState)
{
    NS_ENSURE_ARG_POINTER(aLEDState);

    KeymapWrapper::Modifiers modifier;
    switch (aKeyCode) {
        case NS_VK_CAPS_LOCK:   modifier = KeymapWrapper::CAPS_LOCK;   break;
        case NS_VK_NUM_LOCK:    modifier = KeymapWrapper::NUM_LOCK;    break;
        case NS_VK_SCROLL_LOCK: modifier = KeymapWrapper::SCROLL_LOCK; break;
        default: return NS_ERROR_INVALID_ARG;
    }

    *aLEDState =
        KeymapWrapper::AreModifiersCurrentlyActive(modifier);
    return NS_OK;
}

#if defined(MOZ_X11) && (MOZ_WIDGET_GTK == 2)
 already_AddRefed<gfxASurface>
nsWindow::GetSurfaceForGdkDrawable(GdkDrawable* aDrawable,
                                   const nsIntSize& aSize)
{
    GdkVisual* visual = gdk_drawable_get_visual(aDrawable);
    Screen* xScreen =
        gdk_x11_screen_get_xscreen(gdk_drawable_get_screen(aDrawable));
    Display* xDisplay = DisplayOfScreen(xScreen);
    Drawable xDrawable = gdk_x11_drawable_get_xid(aDrawable);

    nsRefPtr<gfxASurface> result;

    if (visual) {
        Visual* xVisual = gdk_x11_visual_get_xvisual(visual);

        result = new gfxXlibSurface(xDisplay, xDrawable, xVisual,
                                    gfxIntSize(aSize.width, aSize.height));
    } else {
        
        
        XRenderPictFormat *pf = nullptr;
        switch (gdk_drawable_get_depth(aDrawable)) {
            case 32:
                pf = XRenderFindStandardFormat(xDisplay, PictStandardARGB32);
                break;
            case 24:
                pf = XRenderFindStandardFormat(xDisplay, PictStandardRGB24);
                break;
            default:
                NS_ERROR("Don't know how to handle the given depth!");
                break;
        }

        result = new gfxXlibSurface(xScreen, xDrawable, pf,
                                    gfxIntSize(aSize.width, aSize.height));
    }

    return result.forget();
}
#endif

TemporaryRef<DrawTarget>
nsWindow::StartRemoteDrawing()
{
  gfxASurface *surf = GetThebesSurface();
  if (!surf) {
    return nullptr;
  }

  IntSize size(surf->GetSize().width, surf->GetSize().height);
  if (size.width <= 0 || size.height <= 0) {
    return nullptr;
  }

  return gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(surf, size);
}


gfxASurface*
nsWindow::GetThebesSurface()
#if (MOZ_WIDGET_GTK == 3)
{
    return GetThebesSurface(nullptr);
}
gfxASurface*
nsWindow::GetThebesSurface(cairo_t *cr)
#endif
{
    if (!mGdkWindow)
        return nullptr;

#ifdef MOZ_X11
    gint width, height;

#if (MOZ_WIDGET_GTK == 2)
    gdk_drawable_get_size(GDK_DRAWABLE(mGdkWindow), &width, &height);
#else
    width = GdkCoordToDevicePixels(gdk_window_get_width(mGdkWindow));
    height = GdkCoordToDevicePixels(gdk_window_get_height(mGdkWindow));
#endif

    
    width = std::min(32767, width);
    height = std::min(32767, height);
    gfxIntSize size(width, height);

    GdkVisual *gdkVisual = gdk_window_get_visual(mGdkWindow);
    Visual* visual = gdk_x11_visual_get_xvisual(gdkVisual);

#  ifdef MOZ_HAVE_SHMIMAGE
    bool usingShm = false;
    if (nsShmImage::UseShm()) {
        
        
        
        mThebesSurface =
            nsShmImage::EnsureShmImage(size,
                                       visual, gdk_visual_get_depth(gdkVisual),
                                       mShmImage);
        usingShm = mThebesSurface != nullptr;
    }
    if (!usingShm)
#  endif  
    {
        mThebesSurface = new gfxXlibSurface
            (GDK_WINDOW_XDISPLAY(mGdkWindow),
             gdk_x11_window_get_xid(mGdkWindow),
             visual,
             size);
    }
#endif 

    
    
    if (mThebesSurface && mThebesSurface->CairoStatus() != 0) {
        mThebesSurface = nullptr;
    }

    return mThebesSurface;
}


bool
nsWindow::GetDragInfo(WidgetMouseEvent* aMouseEvent,
                      GdkWindow** aWindow, gint* aButton,
                      gint* aRootX, gint* aRootY)
{
    if (aMouseEvent->button != WidgetMouseEvent::eLeftButton) {
        
        return false;
    }
    *aButton = 1;

    
    GdkWindow* gdk_window = mGdkWindow;
    if (!gdk_window) {
        return false;
    }
#ifdef DEBUG
    
    
    
    if (!GDK_IS_WINDOW(gdk_window)) {
        MOZ_ASSERT(false, "must really be window");
    }
#endif

    
    gdk_window = gdk_window_get_toplevel(gdk_window);
    MOZ_ASSERT(gdk_window,
               "gdk_window_get_toplevel should not return null");
    *aWindow = gdk_window;

    if (!aMouseEvent->widget) {
        return false;
    }

    
    
    
    
    
    LayoutDeviceIntPoint offset = aMouseEvent->widget->WidgetToScreenOffset();
    *aRootX = aMouseEvent->refPoint.x + offset.x;
    *aRootY = aMouseEvent->refPoint.y + offset.y;

    return true;
}

NS_IMETHODIMP
nsWindow::BeginMoveDrag(WidgetMouseEvent* aEvent)
{
    MOZ_ASSERT(aEvent, "must have event");
    MOZ_ASSERT(aEvent->mClass == eMouseEventClass,
               "event must have correct struct type");

    GdkWindow *gdk_window;
    gint button, screenX, screenY;
    if (!GetDragInfo(aEvent, &gdk_window, &button, &screenX, &screenY)) {
        return NS_ERROR_FAILURE;
    }

    
    screenX = DevicePixelsToGdkCoordRoundDown(screenX);
    screenY = DevicePixelsToGdkCoordRoundDown(screenY);
    gdk_window_begin_move_drag(gdk_window, button, screenX, screenY,
                               aEvent->time);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::BeginResizeDrag(WidgetGUIEvent* aEvent,
                          int32_t aHorizontal,
                          int32_t aVertical)
{
    NS_ENSURE_ARG_POINTER(aEvent);

    if (aEvent->mClass != eMouseEventClass) {
        
        return NS_ERROR_INVALID_ARG;
    }

    GdkWindow *gdk_window;
    gint button, screenX, screenY;
    if (!GetDragInfo(aEvent->AsMouseEvent(), &gdk_window, &button,
                     &screenX, &screenY)) {
        return NS_ERROR_FAILURE;
    }

    
    GdkWindowEdge window_edge;
    if (aVertical < 0) {
        if (aHorizontal < 0) {
            window_edge = GDK_WINDOW_EDGE_NORTH_WEST;
        } else if (aHorizontal == 0) {
            window_edge = GDK_WINDOW_EDGE_NORTH;
        } else {
            window_edge = GDK_WINDOW_EDGE_NORTH_EAST;
        }
    } else if (aVertical == 0) {
        if (aHorizontal < 0) {
            window_edge = GDK_WINDOW_EDGE_WEST;
        } else if (aHorizontal == 0) {
            return NS_ERROR_INVALID_ARG;
        } else {
            window_edge = GDK_WINDOW_EDGE_EAST;
        }
    } else {
        if (aHorizontal < 0) {
            window_edge = GDK_WINDOW_EDGE_SOUTH_WEST;
        } else if (aHorizontal == 0) {
            window_edge = GDK_WINDOW_EDGE_SOUTH;
        } else {
            window_edge = GDK_WINDOW_EDGE_SOUTH_EAST;
        }
    }

    
    gdk_window_begin_resize_drag(gdk_window, window_edge, button,
                                 screenX, screenY, aEvent->time);

    return NS_OK;
}

nsIWidget::LayerManager*
nsWindow::GetLayerManager(PLayerTransactionChild* aShadowManager,
                          LayersBackend aBackendHint,
                          LayerManagerPersistence aPersistence,
                          bool* aAllowRetaining)
{
    if (!mLayerManager && eTransparencyTransparent == GetTransparencyMode()) {
        mLayerManager = CreateBasicLayerManager();
    }

    return nsBaseWidget::GetLayerManager(aShadowManager, aBackendHint,
                                         aPersistence, aAllowRetaining);
}

void
nsWindow::ClearCachedResources()
{
    if (mLayerManager &&
        mLayerManager->GetBackendType() == mozilla::layers::LayersBackend::LAYERS_BASIC) {
        mLayerManager->ClearCachedResources();
    }

    GList* children = gdk_window_peek_children(mGdkWindow);
    for (GList* list = children; list; list = list->next) {
        nsWindow* window = get_window_for_gdk_window(GDK_WINDOW(list->data));
        if (window) {
            window->ClearCachedResources();
        }
    }
}

gint
nsWindow::GdkScaleFactor()
{
#if (MOZ_WIDGET_GTK >= 3)
    
    static auto sGdkWindowGetScaleFactorPtr = (gint (*)(GdkWindow*))
        dlsym(RTLD_DEFAULT, "gdk_window_get_scale_factor");
    if (sGdkWindowGetScaleFactorPtr)
        return (*sGdkWindowGetScaleFactorPtr)(mGdkWindow);
#endif
    return 1;
}


gint
nsWindow::DevicePixelsToGdkCoordRoundUp(int pixels) {
    gint scale = GdkScaleFactor();
    return (pixels + scale - 1) / scale;
}

gint
nsWindow::DevicePixelsToGdkCoordRoundDown(int pixels) {
    gint scale = GdkScaleFactor();
    return pixels / scale;
}

GdkPoint
nsWindow::DevicePixelsToGdkPointRoundDown(nsIntPoint point) {
    gint scale = GdkScaleFactor();
    return { point.x / scale, point.y / scale };
}

GdkRectangle
nsWindow::DevicePixelsToGdkRectRoundOut(nsIntRect rect) {
    gint scale = GdkScaleFactor();
    int x = rect.x / scale;
    int y = rect.y / scale;
    int right = (rect.x + rect.width + scale - 1) / scale;
    int bottom = (rect.y + rect.height + scale - 1) / scale;
    return { x, y, right - x, bottom - y };
}

int
nsWindow::GdkCoordToDevicePixels(gint coord) {
    return coord * GdkScaleFactor();
}

LayoutDeviceIntPoint
nsWindow::GdkPointToDevicePixels(GdkPoint point) {
    gint scale = GdkScaleFactor();
    return LayoutDeviceIntPoint(point.x * scale,
                                point.y * scale);
}

nsIntRect
nsWindow::GdkRectToDevicePixels(GdkRectangle rect) {
    gint scale = GdkScaleFactor();
    return nsIntRect(rect.x * scale,
                     rect.y * scale,
                     rect.width * scale,
                     rect.height * scale);
}

nsresult
nsWindow::SynthesizeNativeMouseEvent(LayoutDeviceIntPoint aPoint,
                                     uint32_t aNativeMessage,
                                     uint32_t aModifierFlags,
                                     nsIObserver* aObserver)
{
  AutoObserverNotifier notifier(aObserver, "mouseevent");

  if (!mGdkWindow) {
    return NS_OK;
  }

  GdkDisplay* display = gdk_window_get_display(mGdkWindow);

  
  
  
  
  
  if (aNativeMessage == GDK_BUTTON_RELEASE) {
    GdkEvent event;
    memset(&event, 0, sizeof(GdkEvent));
    event.type = (GdkEventType)aNativeMessage;
    event.button.button = 1;
    event.button.window = mGdkWindow;
    event.button.time = GDK_CURRENT_TIME;

#if (MOZ_WIDGET_GTK == 3)
    
    GdkDeviceManager *device_manager = gdk_display_get_device_manager(display);
    event.button.device = gdk_device_manager_get_client_pointer(device_manager);
#endif

    gdk_event_put(&event);
  } else {
    
    
    
    GdkScreen* screen = gdk_window_get_screen(mGdkWindow);
    gdk_display_warp_pointer(display, screen, aPoint.x, aPoint.y);
  }

  return NS_OK;
}
