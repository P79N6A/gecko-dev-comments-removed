







































#ifdef MOZ_PLATFORM_MAEMO

#define MAEMO_CHANGES
#endif

#include "prlink.h"
#include "nsWindow.h"
#include "nsGTKToolkit.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIDOMNode.h"

#include "nsWidgetsCID.h"
#include "nsDragService.h"
#include "nsIDragSessionGTK.h"

#include "nsGtkKeyUtils.h"
#include "nsGtkCursors.h"

#include <gtk/gtk.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/shape.h>

#ifdef AIX
#include <X11/keysym.h>
#else
#include <X11/XF86keysym.h>
#endif

#include "gtk2xtbin.h"
#endif 
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkprivate.h>

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
#include "nsIObserverService.h"

#include "nsIdleService.h"
#include "nsIPropertyBag2.h"

#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#include "nsIAccessibleDocument.h"
#include "prenv.h"
#include "stdlib.h"
static PRBool sAccessibilityChecked = PR_FALSE;

PRBool nsWindow::sAccessibilityEnabled = PR_FALSE;
static const char sSysPrefService [] = "@mozilla.org/system-preference-service;1";
static const char sAccEnv [] = "GNOME_ACCESSIBILITY";
static const char sAccessibilityKey [] = "config.use_system_prefs.accessibility";
#endif


#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"
#include "nsILocalFile.h"


#include <gdk/gdk.h>
#include <wchar.h>
#include "imgIContainer.h"
#include "nsGfxCIID.h"
#include "nsImageToPixbuf.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"

extern "C" {
#include "pixman.h"
}
#include "gfxPlatformGtk.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "Layers.h"
#include "LayerManagerOGL.h"
#include "GLContextProvider.h"

#ifdef MOZ_X11
#include "gfxXlibSurface.h"
#endif

#include "nsShmImage.h"

#ifdef MOZ_DFB
extern "C" {
#ifdef MOZ_DIRECT_DEBUG
#define DIRECT_ENABLE_DEBUG
#endif

#include <direct/debug.h>

D_DEBUG_DOMAIN( ns_Window, "nsWindow", "nsWindow" );
}
#include "gfxDirectFBSurface.h"
#define GDK_WINDOW_XWINDOW(_win) _win
#endif

using namespace mozilla;
using mozilla::gl::GLContext;
using mozilla::layers::LayerManagerOGL;



#define MAX_RECTS_IN_REGION 100


static PRBool     check_for_rollup(GdkWindow *aWindow,
                                   gdouble aMouseX, gdouble aMouseY,
                                   PRBool aIsWheel, PRBool aAlwaysRollup);
static PRBool     is_mouse_in_window(GdkWindow* aWindow,
                                     gdouble aMouseX, gdouble aMouseY);
static nsWindow  *get_window_for_gtk_widget(GtkWidget *widget);
static nsWindow  *get_window_for_gdk_window(GdkWindow *window);
static GtkWidget *get_gtk_widget_for_gdk_window(GdkWindow *window);
static GdkCursor *get_gtk_cursor(nsCursor aCursor);

static GdkWindow *get_inner_gdk_window (GdkWindow *aWindow,
                                        gint x, gint y,
                                        gint *retx, gint *rety);

static inline PRBool is_context_menu_key(const nsKeyEvent& inKeyEvent);
static void   key_event_to_context_menu_event(nsMouseEvent &aEvent,
                                              GdkEventKey *aGdkEvent);

static int    is_parent_ungrab_enter(GdkEventCrossing *aEvent);
static int    is_parent_grab_leave(GdkEventCrossing *aEvent);


static gboolean expose_event_cb           (GtkWidget *widget,
                                           GdkEventExpose *event);
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
                                           gpointer *aData);
static void    drag_data_received_event_cb(GtkWidget *aWidget,
                                           GdkDragContext *aDragContext,
                                           gint aX,
                                           gint aY,
                                           GtkSelectionData  *aSelectionData,
                                           guint aInfo,
                                           guint32 aTime,
                                           gpointer aData);

static GdkModifierType gdk_keyboard_get_modifiers();
#ifdef MOZ_X11
static PRBool gdk_keyboard_get_modmap_masks(Display*  aDisplay,
                                            PRUint32* aCapsLockMask,
                                            PRUint32* aNumLockMask,
                                            PRUint32* aScrollLockMask);
#endif 


static nsresult    initialize_prefs        (void);

static void
UpdateLastInputEventTime()
{
  nsCOMPtr<nsIdleService> idleService = do_GetService("@mozilla.org/widget/idleservice;1");
  if (idleService) {
    idleService->ResetIdleTimeOut();
  }
}


nsWindow *nsWindow::sLastDragMotionWindow = NULL;
PRBool nsWindow::sIsDraggingOutOf = PR_FALSE;



guint32   nsWindow::sLastButtonPressTime = 0;


guint32   nsWindow::sLastButtonReleaseTime = 0;

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);


static nsWindow         *gFocusWindow          = NULL;
static PRBool            gBlockActivateEvent   = PR_FALSE;
static PRBool            gGlobalsInitialized   = PR_FALSE;
static PRBool            gRaiseWindows         = PR_TRUE;
static nsWindow         *gPluginFocusWindow    = NULL;

static nsIRollupListener*          gRollupListener;
static nsIMenuRollup*              gMenuRollup;
static nsWeakPtr                   gRollupWindow;
static PRBool                      gConsumeRollupEvent;


#define NS_WINDOW_TITLE_MAX_LENGTH 4095






typedef struct _GdkDisplay GdkDisplay;
typedef GdkDisplay* (*_gdk_display_get_default_fn)(void);

typedef GdkCursor*  (*_gdk_cursor_new_from_pixbuf_fn)(GdkDisplay *display,
                                                      GdkPixbuf *pixbuf,
                                                      gint x,
                                                      gint y);
static _gdk_display_get_default_fn    _gdk_display_get_default;
static _gdk_cursor_new_from_pixbuf_fn _gdk_cursor_new_from_pixbuf;
static PRBool sPixbufCursorChecked;



typedef void (*_gdk_window_set_urgency_hint_fn)(GdkWindow *window,
                                                gboolean urgency);

#define kWindowPositionSlop 20


static GdkCursor *gCursorCache[eCursorCount];


PRBool gDisableNativeTheme = PR_FALSE;

static GtkWidget *gInvisibleContainer = NULL;



static guint gButtonState;



template<class T> static inline gpointer
FuncToGpointer(T aFunction)
{
    return reinterpret_cast<gpointer>
        (reinterpret_cast<uintptr_t>
         
         (reinterpret_cast<void (*)()>(aFunction)));
}



template <>
class nsSimpleRef<pixman_region32> : public pixman_region32 {
protected:
    typedef pixman_region32 RawRef;

    nsSimpleRef() { data = nsnull; }
    nsSimpleRef(const RawRef &aRawRef) : pixman_region32(aRawRef) { }

    static void Release(pixman_region32& region) {
        pixman_region32_fini(&region);
    }
    
    PRBool HaveResource() const { return data != nsnull; }

    pixman_region32& get() { return *this; }
};

nsWindow::nsWindow()
{
    mIsTopLevel       = PR_FALSE;
    mIsDestroyed      = PR_FALSE;
    mNeedsResize      = PR_FALSE;
    mNeedsMove        = PR_FALSE;
    mListenForResizes = PR_FALSE;
    mIsShown          = PR_FALSE;
    mNeedsShow        = PR_FALSE;
    mEnabled          = PR_TRUE;
    mCreated          = PR_FALSE;

    mContainer           = nsnull;
    mGdkWindow           = nsnull;
    mShell               = nsnull;
    mWindowGroup         = nsnull;
    mHasMappedToplevel   = PR_FALSE;
    mIsFullyObscured     = PR_FALSE;
    mRetryPointerGrab    = PR_FALSE;
    mTransientParent     = nsnull;
    mWindowType          = eWindowType_child;
    mSizeState           = nsSizeMode_Normal;
    mLastSizeMode        = nsSizeMode_Normal;

#ifdef MOZ_X11
    mOldFocusWindow      = 0;
#endif 
    mPluginType          = PluginType_NONE;

    if (!gGlobalsInitialized) {
        gGlobalsInitialized = PR_TRUE;

        
        initialize_prefs();
    }

    mLastMotionPressure = 0;

#ifdef ACCESSIBILITY
    mRootAccessible  = nsnull;
#endif

    mIsTransparent = PR_FALSE;
    mTransparencyBitmap = nsnull;

    mTransparencyBitmapWidth  = 0;
    mTransparencyBitmapHeight = 0;

#ifdef MOZ_DFB
    mDFBCursorX     = 0;
    mDFBCursorY     = 0;

    mDFBCursorCount = 0;

    mDFB            = NULL;
    mDFBLayer       = NULL;
#endif
}

nsWindow::~nsWindow()
{
    LOG(("nsWindow::~nsWindow() [%p]\n", (void *)this));
    if (sLastDragMotionWindow == this) {
        sLastDragMotionWindow = NULL;
    }

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = nsnull;

#ifdef MOZ_DFB
    if (mDFBLayer)
         mDFBLayer->Release( mDFBLayer );

    if (mDFB)
         mDFB->Release( mDFB );
#endif

    Destroy();
}

 void
nsWindow::ReleaseGlobals()
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(gCursorCache); ++i) {
    if (gCursorCache[i]) {
      gdk_cursor_unref(gCursorCache[i]);
      gCursorCache[i] = nsnull;
    }
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsBaseWidget,
                             nsISupportsWeakReference)

void
nsWindow::CommonCreate(nsIWidget *aParent, PRBool aListenForResizes)
{
    mParent = aParent;
    mListenForResizes = aListenForResizes;
    mCreated = PR_TRUE;
}

void
nsWindow::InitKeyEvent(nsKeyEvent &aEvent, GdkEventKey *aGdkEvent)
{
    aEvent.keyCode   = GdkKeyCodeToDOMKeyCode(aGdkEvent->keyval);
    
    
    
    
    
    
    
    
    
    guint modifierState = aGdkEvent->state;
    guint changingMask = 0;
    switch (aEvent.keyCode) {
        case NS_VK_SHIFT:
            changingMask = GDK_SHIFT_MASK;
            break;
        case NS_VK_CONTROL:
            changingMask = GDK_CONTROL_MASK;
            break;
        case NS_VK_ALT:
            changingMask = GDK_MOD1_MASK;
            break;
        case NS_VK_META:
            changingMask = GDK_MOD4_MASK;
            break;
    }
    if (changingMask != 0) {
        
        if (aGdkEvent->type == GDK_KEY_PRESS) {
            
            modifierState |= changingMask;
        } else {
            
            
        }
    }
    aEvent.isShift   = (modifierState & GDK_SHIFT_MASK) != 0;
    aEvent.isControl = (modifierState & GDK_CONTROL_MASK) != 0;
    aEvent.isAlt     = (modifierState & GDK_MOD1_MASK) != 0;
    aEvent.isMeta    = (modifierState & GDK_MOD4_MASK) != 0;

    
    
    
    
    aEvent.pluginEvent = (void *)aGdkEvent;

    aEvent.time      = aGdkEvent->time;
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

void
nsWindow::DispatchActivateEvent(void)
{
    NS_ASSERTION(mContainer || mIsDestroyed,
                 "DispatchActivateEvent only intended for container windows");

#ifdef ACCESSIBILITY
    DispatchActivateEventAccessible();
#endif 
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

#ifdef ACCESSIBILITY
    DispatchDeactivateEventAccessible();
#endif 
}



nsresult
nsWindow::DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus)
{
#ifdef DEBUG
    debug_DumpEvent(stdout, aEvent->widget, aEvent,
                    nsCAutoString("something"), 0);
#endif

    aStatus = nsEventStatus_eIgnore;

    
    if (mEventCallback)
        aStatus = (* mEventCallback)(aEvent);

    return NS_OK;
}

void
nsWindow::OnDestroy(void)
{
    if (mOnDestroyCalled)
        return;

    mOnDestroyCalled = PR_TRUE;
    
    
    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;

    
    nsBaseWidget::OnDestroy(); 
    
    
    nsBaseWidget::Destroy();
    mParent = nsnull;

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

    if (!gdk_window_peek_children(gInvisibleContainer->window)) {
        
        
        gtk_widget_destroy(gInvisibleContainer->parent);
        gInvisibleContainer = NULL;
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
        if (widget->parent != aOldWidget)
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
    mIsDestroyed = PR_TRUE;
    mCreated = PR_FALSE;

    
    if (mLayerManager) {
        nsRefPtr<GLContext> gl = nsnull;
        if (mLayerManager->GetBackendType() == LayerManager::LAYERS_OPENGL) {
            LayerManagerOGL *ogllm = static_cast<LayerManagerOGL*>(mLayerManager.get());
            gl = ogllm->gl();
        }

        mLayerManager->Destroy();

        if (gl) {
            gl->MarkDestroyed();
        }
    }
    mLayerManager = nsnull;

    ClearCachedResources();

    g_signal_handlers_disconnect_by_func(gtk_settings_get_default(),
                                         FuncToGpointer(theme_changed_cb),
                                         this);

    
    nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);
    if (static_cast<nsIWidget *>(this) == rollupWidget.get()) {
        if (gRollupListener)
            gRollupListener->Rollup(nsnull, nsnull);
        NS_IF_RELEASE(gMenuRollup);
        gRollupWindow = nsnull;
        gRollupListener = nsnull;
    }

    NativeShow(PR_FALSE);

    if (mIMModule) {
        mIMModule->OnDestroyWindow(this);
    }

    
    if (gFocusWindow == this) {
        LOGFOCUS(("automatically losing focus...\n"));
        gFocusWindow = nsnull;
    }

#ifdef MOZ_X11
    
    if (gPluginFocusWindow == this) {
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }
#endif 

    if (mWindowGroup) {
        g_object_unref(mWindowGroup);
        mWindowGroup = nsnull;
    }

    
    
    mThebesSurface = nsnull;

    if (mDragLeaveTimer) {
        mDragLeaveTimer->Cancel();
        mDragLeaveTimer = nsnull;
    }

    GtkWidget *owningWidget = GetMozContainerWidget();
    if (mShell) {
        gtk_widget_destroy(mShell);
        mShell = nsnull;
        mContainer = nsnull;
        NS_ABORT_IF_FALSE(!mGdkWindow,
                          "mGdkWindow should be NULL when mContainer is destroyed");
    }
    else if (mContainer) {
        gtk_widget_destroy(GTK_WIDGET(mContainer));
        mContainer = nsnull;
        NS_ABORT_IF_FALSE(!mGdkWindow,
                          "mGdkWindow should be NULL when mContainer is destroyed");
    }
    else if (mGdkWindow) {
        
        
        
        
        DestroyChildWindows();

        gdk_window_set_user_data(mGdkWindow, NULL);
        g_object_set_data(G_OBJECT(mGdkWindow), "nsWindow", NULL);
        gdk_window_destroy(mGdkWindow);
        mGdkWindow = nsnull;
    }

    if (gInvisibleContainer && owningWidget == gInvisibleContainer) {
        CheckDestroyInvisibleContainer();
    }

#ifdef ACCESSIBILITY
     if (mRootAccessible) {
         mRootAccessible = nsnull;
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

#ifdef MOZ_PLATFORM_MAEMO
    static float sDPI = 0;

    if (!sDPI) {
        
        nsCOMPtr<nsIPropertyBag2> infoService = do_GetService("@mozilla.org/system-info;1");
        NS_ASSERTION(infoService, "Could not find a system info service");

        nsCString deviceType;
        infoService->GetPropertyAsACString(NS_LITERAL_STRING("device"), deviceType);
        if (deviceType.EqualsLiteral("Nokia N900")) {
            sDPI = 265.0f;
        } else if (deviceType.EqualsLiteral("Nokia N8xx")) {
            sDPI = 225.0f;
        } else {
            
            NS_WARNING("Unknown device - using default DPI");
            sDPI = 96.0f;
        }
    }
    return sDPI;
#else
    Display *dpy = GDK_DISPLAY();
    int defaultScreen = DefaultScreen(dpy);
    double heightInches = DisplayHeightMM(dpy, defaultScreen)/MM_PER_INCH_FLOAT;
    if (heightInches < 0.25) {
        
        return 96.0f;
    }
    return float(DisplayHeight(dpy, defaultScreen)/heightInches);
#endif
}

NS_IMETHODIMP
nsWindow::SetParent(nsIWidget *aNewParent)
{
    if (mContainer || !mGdkWindow || !mParent) {
        NS_NOTREACHED("nsWindow::SetParent - reparenting a non-child window");
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    NS_ASSERTION(!mTransientParent, "child widget with transient parent");

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;
    mParent->RemoveChild(this);

    mParent = aNewParent;

    GtkWidget* oldContainer = GetMozContainerWidget();
    if (!oldContainer) {
        
        
        NS_ABORT_IF_FALSE(GDK_WINDOW_OBJECT(mGdkWindow)->destroyed,
                          "live GdkWindow with no widget");
        return NS_OK;
    }

    if (aNewParent) {
        aNewParent->AddChild(this);
        ReparentNativeWidget(aNewParent);
    } else {
        
        
        
        
        GtkWidget* newContainer = EnsureInvisibleContainer();
        GdkWindow* newParentWindow = newContainer->window;
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
        
        
        NS_ABORT_IF_FALSE(GDK_WINDOW_OBJECT(mGdkWindow)->destroyed,
                          "live GdkWindow with no widget");
        return NS_OK;
    }
    NS_ABORT_IF_FALSE(!GDK_WINDOW_OBJECT(mGdkWindow)->destroyed,
                      "destroyed GdkWindow with widget");
    
    nsWindow* newParent = static_cast<nsWindow*>(aNewParent);
    GdkWindow* newParentWindow = newParent->mGdkWindow;
    GtkWidget* newContainer = NULL;
    if (newParentWindow) {
        newContainer = get_gtk_widget_for_gdk_window(newParentWindow);
    }

    if (mTransientParent) {
      GtkWindow* topLevelParent =
          GTK_WINDOW(gtk_widget_get_toplevel(newContainer));
      gtk_window_set_transient_for(GTK_WINDOW(mShell), topLevelParent);
      mTransientParent = topLevelParent;
      if (mWindowGroup) {
          g_object_unref(mWindowGroup);
          mWindowGroup = NULL;
      }
      if (mTransientParent->group) {
          gtk_window_group_add_window(mTransientParent->group,
                                      GTK_WINDOW(mShell));
          mWindowGroup = mTransientParent->group;
          g_object_ref(mWindowGroup);
      }
      else if (GTK_WINDOW(mShell)->group) {
          gtk_window_group_remove_window(GTK_WINDOW(mShell)->group,
                                         GTK_WINDOW(mShell));
      }
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
        
        NS_ABORT_IF_FALSE(!aNewParentWindow ||
                          GDK_WINDOW_OBJECT(aNewParentWindow)->destroyed,
                          "live GdkWindow with no widget");
        Destroy();
    } else {
        if (aNewContainer != aOldContainer) {
            NS_ABORT_IF_FALSE(!GDK_WINDOW_OBJECT(aNewParentWindow)->destroyed,
                              "destroyed GdkWindow with widget");
            SetWidgetForHierarchy(mGdkWindow, aOldContainer, aNewContainer);
        }

        if (!mIsTopLevel) {
            gdk_window_reparent(mGdkWindow, aNewParentWindow, mBounds.x,
                                mBounds.y);
        }
    }

    nsWindow* newParent = static_cast<nsWindow*>(aNewParent);
    PRBool parentHasMappedToplevel =
        newParent && newParent->mHasMappedToplevel;
    if (mHasMappedToplevel != parentHasMappedToplevel) {
        SetHasMappedToplevel(parentHasMappedToplevel);
    }
}

NS_IMETHODIMP
nsWindow::SetModal(PRBool aModal)
{
    LOG(("nsWindow::SetModal [%p] %d\n", (void *)this, aModal));
    if (mIsDestroyed)
        return aModal ? NS_ERROR_NOT_AVAILABLE : NS_OK;
    if (!mIsTopLevel || !mShell)
        return NS_ERROR_FAILURE;
    gtk_window_set_modal(GTK_WINDOW(mShell), aModal ? TRUE : FALSE);
    return NS_OK;
}


NS_IMETHODIMP
nsWindow::IsVisible(PRBool& aState)
{
    aState = mIsShown;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY)
{
    if (mIsTopLevel && mShell) {
        PRInt32 screenWidth = gdk_screen_width();
        PRInt32 screenHeight = gdk_screen_height();
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
nsWindow::Show(PRBool aState)
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
        mNeedsShow = PR_TRUE;
        return NS_OK;
    }

    
    if (!aState)
        mNeedsShow = PR_FALSE;

    
    
    if (aState) {
        if (mNeedsMove) {
            NativeResize(mBounds.x, mBounds.y, mBounds.width, mBounds.height,
                         PR_FALSE);
        } else if (mNeedsResize) {
            NativeResize(mBounds.width, mBounds.height, PR_FALSE);
        }
    }

    NativeShow(aState);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
    mBounds.SizeTo(GetSafeWindowSize(nsIntSize(aWidth, aHeight)));

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
                NativeShow(PR_TRUE);
        }
        else {
            
            
            
            
            
            
            if (!mNeedsShow) {
                mNeedsShow = PR_TRUE;
                NativeShow(PR_FALSE);
            }
        }
    }
    
    
    else {
        if (AreBoundsSane() && mListenForResizes) {
            
            
            
            NativeResize(aWidth, aHeight, aRepaint);
        }
        else {
            mNeedsResize = PR_TRUE;
        }
    }

    
    if (mIsTopLevel || mListenForResizes) {
        nsIntRect rect(mBounds.x, mBounds.y, aWidth, aHeight);
        nsEventStatus status;
        DispatchResizeEvent(rect, status);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight,
                       PRBool aRepaint)
{
    mBounds.x = aX;
    mBounds.y = aY;
    mBounds.SizeTo(GetSafeWindowSize(nsIntSize(aWidth, aHeight)));

    mNeedsMove = PR_TRUE;

    if (!mCreated)
        return NS_OK;

    
    
    

    
    if (mIsShown) {
        
        if (AreBoundsSane()) {
            
            NativeResize(aX, aY, aWidth, aHeight, aRepaint);
            
            if (mNeedsShow)
                NativeShow(PR_TRUE);
        }
        else {
            
            
            
            
            
            
            if (!mNeedsShow) {
                mNeedsShow = PR_TRUE;
                NativeShow(PR_FALSE);
            }
        }
    }
    
    
    else {
        if (AreBoundsSane() && mListenForResizes){
            
            
            
            NativeResize(aX, aY, aWidth, aHeight, aRepaint);
        }
        else {
            mNeedsResize = PR_TRUE;
        }
    }

    if (mIsTopLevel || mListenForResizes) {
        
        nsIntRect rect(aX, aY, aWidth, aHeight);
        nsEventStatus status;
        DispatchResizeEvent(rect, status);
    }

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



NS_IMETHODIMP
nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
    LOG(("nsWindow::Move [%p] %d %d\n", (void *)this,
         aX, aY));

    if (mWindowType == eWindowType_toplevel ||
        mWindowType == eWindowType_dialog) {
        SetSizeMode(nsSizeMode_Normal);
    }

    
    
    
    if (aX == mBounds.x && aY == mBounds.y &&
        mWindowType != eWindowType_popup)
        return NS_OK;

    

    mBounds.x = aX;
    mBounds.y = aY;

    if (!mCreated)
        return NS_OK;

    mNeedsMove = PR_FALSE;

    if (mIsTopLevel) {
        gtk_window_move(GTK_WINDOW(mShell), aX, aY);
    }
    else if (mGdkWindow) {
        gdk_window_move(mGdkWindow, aX, aY);
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
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetSizeMode(PRInt32 aMode)
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
        MakeFullScreen(PR_TRUE);
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
    nsCOMPtr<nsIToolkit> toolkit;
    NS_GetCurrentToolkit(getter_AddRefs(toolkit));
    if (!toolkit)
        return;

    nsGTKToolkit* GTKToolkit = static_cast<nsGTKToolkit*>
                                          (static_cast<nsIToolkit*>(toolkit));
    nsCAutoString desktopStartupID;
    GTKToolkit->GetDesktopStartupID(&desktopStartupID);
    if (desktopStartupID.IsEmpty()) {
        
        
        
        
        PRUint32 timestamp = GTKToolkit->GetFocusTimestamp();
        if (timestamp) {
            gdk_window_focus(aWindow->window, timestamp);
            GTKToolkit->SetFocusTimestamp(0);
        }
        return;
    }

#ifdef MOZ_ENABLE_STARTUP_NOTIFICATION
    GdkDrawable* drawable = GDK_DRAWABLE(aWindow->window);
    GtkWindow* win = GTK_WINDOW(aWindow);
    if (!win) {
        NS_WARNING("Passed in widget was not a GdkWindow!");
        return;
    }
    GdkScreen* screen = gtk_window_get_screen(win);
    SnDisplay* snd =
        sn_display_new(gdk_x11_drawable_get_xdisplay(drawable), nsnull, nsnull);
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
        PRLibrary* gtkLibrary;
        SetUserTimeFunc setUserTimeFunc = (SetUserTimeFunc)
            PR_FindFunctionSymbolAndLibrary("gdk_x11_window_set_user_time", &gtkLibrary);
        if (setUserTimeFunc) {
            setUserTimeFunc(aWindow->window, sn_launchee_context_get_timestamp(ctx));
            PR_UnloadLibrary(gtkLibrary);
        }
    }

    sn_launchee_context_setup_window(ctx, gdk_x11_drawable_get_xid(drawable));
    sn_launchee_context_complete(ctx);

    sn_launchee_context_unref(ctx);
    sn_display_unref(snd);
#endif

    GTKToolkit->SetDesktopStartupID(EmptyCString());
}

NS_IMETHODIMP
nsWindow::SetFocus(PRBool aRaise)
{
    
    

    LOGFOCUS(("  SetFocus %d [%p]\n", aRaise, (void *)this));

    GtkWidget *owningWidget = GetMozContainerWidget();
    if (!owningWidget)
        return NS_ERROR_FAILURE;

    
    
    GtkWidget *toplevelWidget = gtk_widget_get_toplevel(owningWidget);

    if (gRaiseWindows && aRaise && toplevelWidget &&
        !GTK_WIDGET_HAS_FOCUS(owningWidget) &&
        !GTK_WIDGET_HAS_FOCUS(toplevelWidget)) {
        GtkWidget* top_window = nsnull;
        GetToplevelWidget(&top_window);
        if (top_window && (GTK_WIDGET_VISIBLE(top_window)))
        {
            gdk_window_show_unraised(top_window->window);
            
            SetUrgencyHint(top_window, PR_FALSE);
        }
    }

    nsRefPtr<nsWindow> owningWindow = get_window_for_gtk_widget(owningWidget);
    if (!owningWindow)
        return NS_ERROR_FAILURE;

    if (aRaise) {
        

        
        
        
        if (gRaiseWindows && owningWindow->mIsShown && owningWindow->mShell &&
            !gtk_window_is_active(GTK_WINDOW(owningWindow->mShell))) {

            LOGFOCUS(("  requesting toplevel activation [%p]\n", (void *)this));
            NS_ASSERTION(owningWindow->mWindowType != eWindowType_popup
                         || mParent,
                         "Presenting an override-redirect window");
            gtk_window_present(GTK_WINDOW(owningWindow->mShell));
        }

        return NS_OK;
    }

    
    

    
    
    
    
    if (!gtk_widget_is_focus(owningWidget)) {
        
        
        
        
        gBlockActivateEvent = PR_TRUE;
        gtk_widget_grab_focus(owningWidget);
        gBlockActivateEvent = PR_FALSE;
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
        gdk_window_get_root_origin(GTK_WIDGET(mContainer)->window, &x, &y);
        aRect.MoveTo(x, y);
    }
    else {
        aRect.MoveTo(WidgetToScreenOffset());
    }
    aRect.SizeTo(mBounds.Size());
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
    
    
    if (!mContainer && mGdkWindow) {
        nsWindow *window = GetContainerWindow();
        if (!window)
            return NS_ERROR_FAILURE;

        return window->SetCursor(aCursor);
    }

    
    if (aCursor != mCursor) {
        GdkCursor *newCursor = NULL;

        newCursor = get_gtk_cursor(aCursor);

        if (nsnull != newCursor) {
            mCursor = aCursor;

            if (!mContainer)
                return NS_OK;

            gdk_window_set_cursor(GTK_WIDGET(mContainer)->window, newCursor);
        }
    }

    return NS_OK;
}


static
PRUint8* Data32BitTo1Bit(PRUint8* aImageData,
                         PRUint32 aImageBytesPerRow,
                         PRUint32 aWidth, PRUint32 aHeight)
{
  PRUint32 outBpr = (aWidth + 7) / 8;

  PRUint8* outData = new PRUint8[outBpr * aHeight];
  if (!outData)
      return NULL;

  PRUint8 *outRow = outData,
          *imageRow = aImageData;

  for (PRUint32 curRow = 0; curRow < aHeight; curRow++) {
      PRUint8 *irow = imageRow;
      PRUint8 *orow = outRow;
      PRUint8 imagePixels = 0;
      PRUint8 offset = 0;

      for (PRUint32 curCol = 0; curCol < aWidth; curCol++) {
          PRUint8 r = *imageRow++,
                  g = *imageRow++,
                  b = *imageRow++;
               imageRow++;

          if ((r + b + g) < 3 * 128)
              imagePixels |= (1 << offset);

          if (offset == 7) {
              *outRow++ = imagePixels;
              offset = 0;
              imagePixels = 0;
          } else {
              offset++;
          }
      }
      if (offset != 0)
          *outRow++ = imagePixels;

      imageRow = irow + aImageBytesPerRow;
      outRow = orow + outBpr;
  }

  return outData;
}



NS_IMETHODIMP
nsWindow::SetCursor(imgIContainer* aCursor,
                    PRUint32 aHotspotX, PRUint32 aHotspotY)
{
    
    
    if (!mContainer && mGdkWindow) {
        nsWindow *window = GetContainerWindow();
        if (!window)
            return NS_ERROR_FAILURE;

        return window->SetCursor(aCursor, aHotspotX, aHotspotY);
    }

    if (!sPixbufCursorChecked) {
        PRLibrary* lib;
        _gdk_cursor_new_from_pixbuf = (_gdk_cursor_new_from_pixbuf_fn)
            PR_FindFunctionSymbolAndLibrary("gdk_cursor_new_from_pixbuf", &lib);
        if (lib) {
            
            PR_UnloadLibrary(lib);
            lib = nsnull;
        }
        _gdk_display_get_default = (_gdk_display_get_default_fn)
            PR_FindFunctionSymbolAndLibrary("gdk_display_get_default", &lib);
        if (lib) {
            
            PR_UnloadLibrary(lib);
            lib = nsnull;
        }
        sPixbufCursorChecked = PR_TRUE;
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

    GdkCursor* cursor;
    if (!_gdk_cursor_new_from_pixbuf || !_gdk_display_get_default) {
        
        GdkPixmap* mask = gdk_pixmap_new(NULL, width, height, 1);
        if (!mask) {
            g_object_unref(pixbuf);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        PRUint8* data = Data32BitTo1Bit(gdk_pixbuf_get_pixels(pixbuf),
                                        gdk_pixbuf_get_rowstride(pixbuf),
                                        width, height);
        if (!data) {
            g_object_unref(mask);
            g_object_unref(pixbuf);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        GdkPixmap* image = gdk_bitmap_create_from_data(NULL, (const gchar*)data, width,
                                                       height);
        delete[] data;
        if (!image) {
            g_object_unref(mask);
            g_object_unref(pixbuf);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        gdk_pixbuf_render_threshold_alpha(pixbuf, mask, 0, 0, 0, 0, width,
                                          height, 1);

        GdkColor fg = { 0, 0, 0, 0 }; 
        GdkColor bg = { 0, 0xFFFF, 0xFFFF, 0xFFFF }; 

        cursor = gdk_cursor_new_from_pixmap(image, mask, &fg, &bg, aHotspotX,
                                            aHotspotY);
        g_object_unref(image);
        g_object_unref(mask);
    } else {
        
        cursor = _gdk_cursor_new_from_pixbuf(_gdk_display_get_default(),
                                             pixbuf,
                                             aHotspotX, aHotspotY);
    }
    g_object_unref(pixbuf);
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    if (cursor) {
        if (mContainer) {
            gdk_window_set_cursor(GTK_WIDGET(mContainer)->window, cursor);
            rv = NS_OK;
        }
        gdk_cursor_unref(cursor);
    }

    return rv;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsIntRect &aRect,
                     PRBool           aIsSynchronous)
{
    if (!mGdkWindow)
        return NS_OK;

    GdkRectangle rect;
    rect.x = aRect.x;
    rect.y = aRect.y;
    rect.width = aRect.width;
    rect.height = aRect.height;

    LOGDRAW(("Invalidate (rect) [%p]: %d %d %d %d (sync: %d)\n", (void *)this,
             rect.x, rect.y, rect.width, rect.height, aIsSynchronous));

    gdk_window_invalidate_rect(mGdkWindow, &rect, FALSE);
    if (aIsSynchronous)
        gdk_window_process_updates(mGdkWindow, FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Update()
{
    if (!mGdkWindow)
        return NS_OK;

    LOGDRAW(("Update [%p] %p\n", this, mGdkWindow));

    gdk_window_process_updates(mGdkWindow, FALSE);
    
    gdk_display_flush(gdk_drawable_get_display(GDK_DRAWABLE(mGdkWindow)));
    return NS_OK;
}

void*
nsWindow::GetNativeData(PRUint32 aDataType)
{
    switch (aDataType) {
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_WIDGET: {
        if (!mGdkWindow)
            return nsnull;

        return mGdkWindow;
        break;
    }

    case NS_NATIVE_PLUGIN_PORT:
        return SetupPluginPort();
        break;

    case NS_NATIVE_DISPLAY:
#ifdef MOZ_X11
        return GDK_DISPLAY();
#else
        return nsnull;
#endif 
        break;

    case NS_NATIVE_GRAPHIC: {
        NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
        return (void *)static_cast<nsGTKToolkit *>(mToolkit)->GetSharedGC();
        break;
    }

    case NS_NATIVE_SHELLWIDGET:
        return (void *) mShell;

    default:
        NS_WARNING("nsWindow::GetNativeData called with bad value");
        return nsnull;
    }
}

NS_IMETHODIMP
nsWindow::SetTitle(const nsAString& aTitle)
{
    if (!mShell)
        return NS_OK;

    
#define UTF8_FOLLOWBYTE(ch) (((ch) & 0xC0) == 0x80)
    NS_ConvertUTF16toUTF8 titleUTF8(aTitle);
    if (titleUTF8.Length() > NS_WINDOW_TITLE_MAX_LENGTH) {
        
        
        PRUint32 len = NS_WINDOW_TITLE_MAX_LENGTH;
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
    gint x = 0, y = 0;

    if (mGdkWindow) {
        gdk_window_get_origin(mGdkWindow, &x, &y);
    }

    return nsIntPoint(x, y);
}

NS_IMETHODIMP
nsWindow::EnableDragDrop(PRBool aEnable)
{
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureMouse(PRBool aCapture)
{
    LOG(("CaptureMouse %p\n", (void *)this));

    if (!mGdkWindow)
        return NS_OK;

    GtkWidget *widget = GetMozContainerWidget();
    if (!widget)
        return NS_ERROR_FAILURE;

    if (aCapture) {
        gtk_grab_add(widget);
        GrabPointer();
    }
    else {
        ReleaseGrabs();
        gtk_grab_remove(widget);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CaptureRollupEvents(nsIRollupListener *aListener,
                              nsIMenuRollup     *aMenuRollup,
                              PRBool             aDoCapture,
                              PRBool             aConsumeRollupEvent)
{
    if (!mGdkWindow)
        return NS_OK;

    GtkWidget *widget = GetMozContainerWidget();
    if (!widget)
        return NS_ERROR_FAILURE;

    LOG(("CaptureRollupEvents %p\n", (void *)this));

    if (aDoCapture) {
        gConsumeRollupEvent = aConsumeRollupEvent;
        gRollupListener = aListener;
        NS_IF_RELEASE(gMenuRollup);
        gMenuRollup = aMenuRollup;
        NS_IF_ADDREF(aMenuRollup);
        gRollupWindow = do_GetWeakReference(static_cast<nsIWidget*>
                                                       (this));
        
        if (!nsWindow::DragInProgress()) {
            gtk_grab_add(widget);
            GrabPointer();
        }
    }
    else {
        if (!nsWindow::DragInProgress()) {
            ReleaseGrabs();
        }
        
        
        
        gtk_grab_remove(widget);
        gRollupListener = nsnull;
        NS_IF_RELEASE(gMenuRollup);
        gRollupWindow = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetAttention(PRInt32 aCycleCount)
{
    LOG(("nsWindow::GetAttention [%p]\n", (void *)this));

    GtkWidget* top_window = nsnull;
    GtkWidget* top_focused_window = nsnull;
    GetToplevelWidget(&top_window);
    if (gFocusWindow)
        gFocusWindow->GetToplevelWidget(&top_focused_window);

    
    if (top_window && (GTK_WIDGET_VISIBLE(top_window)) &&
        top_window != top_focused_window) {
        SetUrgencyHint(top_window, PR_TRUE);
    }

    return NS_OK;
}

PRBool
nsWindow::HasPendingInputEvent()
{
    
    
    
    
    
    PRBool haveEvent;
#ifdef MOZ_X11
    XEvent ev;
    haveEvent =
        XCheckMaskEvent(GDK_DISPLAY(),
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
        XPutBackEvent(GDK_DISPLAY(), &ev);
    }
#else
    haveEvent = PR_FALSE;
#endif
    return haveEvent;
}

#if 0
#ifdef DEBUG


#define CAPS_LOCK_IS_ON \
(gdk_keyboard_get_modifiers() & GDK_LOCK_MASK)

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

  gdk_window_get_geometry(aGdkWindow,
                          NULL,
                          NULL,
                          &width,
                          &height,
                          NULL);

  gdk_window_get_origin (aGdkWindow,
                         &x,
                         &y);

  gc = gdk_gc_new(GDK_ROOT_PARENT());

  white.pixel = WhitePixel(gdk_display,DefaultScreen(gdk_display));

  gdk_gc_set_foreground(gc,&white);
  gdk_gc_set_function(gc,GDK_XOR);
  gdk_gc_set_subwindow(gc,GDK_INCLUDE_INFERIORS);

  gdk_region_offset(aRegion, x, y);
  gdk_gc_set_clip_region(gc, aRegion);

  



  for (i = 0; i < aTimes * 2; i++)
  {
    gdk_draw_rectangle(GDK_ROOT_PARENT(),
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

static void
DispatchDidPaint(nsIWidget* aWidget)
{
    nsEventStatus status;
    nsPaintEvent didPaintEvent(PR_TRUE, NS_DID_PAINT, aWidget);
    aWidget->DispatchEvent(&didPaintEvent, status);
}

gboolean
nsWindow::OnExposeEvent(GtkWidget *aWidget, GdkEventExpose *aEvent)
{
    if (mIsDestroyed) {
        LOG(("Expose event on destroyed window [%p] window %p\n",
             (void *)this, (void *)aEvent->window));
        return FALSE;
    }

    
    if (!mGdkWindow || mIsFullyObscured || !mHasMappedToplevel)
        return FALSE;

    
    
    {
        nsEventStatus status;
        nsPaintEvent willPaintEvent(PR_TRUE, NS_WILL_PAINT, this);
        willPaintEvent.willSendDidPaint = PR_TRUE;
        DispatchEvent(&willPaintEvent, status);

        
        
        if (!mGdkWindow)
            return TRUE;
    }

    nsPaintEvent event(PR_TRUE, NS_PAINT, this);
    event.refPoint.x = aEvent->area.x;
    event.refPoint.y = aEvent->area.y;
    event.willSendDidPaint = PR_TRUE;

    GdkRectangle *rects;
    gint nrects;
    gdk_region_get_rectangles(aEvent->region, &rects, &nrects);
    if (NS_UNLIKELY(!rects)) 
        return FALSE;

    if (nrects > MAX_RECTS_IN_REGION) {
        
        rects[0] = aEvent->area;
        nrects = 1;
    }

    LOGDRAW(("sending expose event [%p] %p 0x%lx (rects follow):\n",
             (void *)this, (void *)aEvent->window,
             GDK_WINDOW_XWINDOW(aEvent->window)));

    GdkRectangle *r;
    GdkRectangle *r_end = rects + nrects;
    for (r = rects; r < r_end; ++r) {
        event.region.Or(event.region, nsIntRect(r->x, r->y, r->width, r->height));
        LOGDRAW(("\t%d %d %d %d\n", r->x, r->y, r->width, r->height));
    }

    
    
    
    event.region.And(event.region,
                     nsIntRect(0, 0, mBounds.width, mBounds.height));

    PRBool translucent = eTransparencyTransparent == GetTransparencyMode();
    if (!translucent) {
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
                for (PRUint32 i = 0; i < clipRects.Length(); ++i) {
                    nsIntRect r = clipRects[i] + bounds.TopLeft();
                    event.region.Sub(event.region, r);
                }
            }
            children = children->next;
        }
    }

    if (event.region.IsEmpty()) {
        g_free(rects);
        return TRUE;
    }

    if (GetLayerManager()->GetBackendType() == LayerManager::LAYERS_OPENGL)
    {
        LayerManagerOGL *manager = static_cast<LayerManagerOGL*>(GetLayerManager());
        manager->SetClippingRegion(event.region);

        nsEventStatus status;
        DispatchEvent(&event, status);

        g_free(rects);

        DispatchDidPaint(this);

        return TRUE;
    }
            
    nsRefPtr<gfxContext> ctx = new gfxContext(GetThebesSurface());

#ifdef MOZ_DFB
    gfxPlatformGtk::SetGdkDrawable(ctx->OriginalSurface(),
                                   GDK_DRAWABLE(mGdkWindow));

    
    gfxUtils::ClipToRegion(ctx, event.region);

    BasicLayerManager::BufferMode layerBuffering =
        BasicLayerManager::BUFFER_NONE;
#endif

#ifdef MOZ_X11
    nsIntRect boundsRect; 

    ctx->NewPath();
    if (translucent) {
        
        
        
        
        boundsRect = event.region.GetBounds();
        ctx->Rectangle(gfxRect(boundsRect.x, boundsRect.y,
                               boundsRect.width, boundsRect.height));
    } else {
        gfxUtils::PathFromRegion(ctx, event.region);
    }
    ctx->Clip();

    BasicLayerManager::BufferMode layerBuffering;
    if (translucent) {
        
        
        
        layerBuffering = BasicLayerManager::BUFFER_NONE;
        ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
#ifdef MOZ_HAVE_SHMIMAGE
    } else if (nsShmImage::UseShm()) {
        
        layerBuffering = BasicLayerManager::BUFFER_NONE;
#endif
    } else {
        
        layerBuffering = BasicLayerManager::BUFFER_BUFFERED;
    }

#if 0
    
    
    
#ifdef DEBUG
    
    if (0 && WANT_PAINT_FLASHING && aEvent->window)
        gdk_window_flash(aEvent->window, 1, 100, aEvent->region);
#endif
#endif

#endif 

    nsEventStatus status;
    {
      AutoLayerManagerSetup setupLayerManager(this, ctx, layerBuffering);
      DispatchEvent(&event, status);
    }

#ifdef MOZ_X11
    
    
    if (translucent) {
        if (NS_LIKELY(!mIsDestroyed)) {
            if (status != nsEventStatus_eIgnore) {
                nsRefPtr<gfxPattern> pattern = ctx->PopGroup();

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

                    UpdateTranslucentWindowAlphaInternal(nsIntRect(boundsRect.x, boundsRect.y,
                                                                   boundsRect.width, boundsRect.height),
                                                         img->Data(), img->Stride());
                }

                ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
                ctx->SetPattern(pattern);
                ctx->Paint();
            }
        }
    }
#  ifdef MOZ_HAVE_SHMIMAGE
    if (nsShmImage::UseShm() && NS_LIKELY(!mIsDestroyed)) {
        mShmImage->Put(mGdkWindow, rects, r_end);
    }
#  endif  
#endif 

    g_free(rects);

    DispatchDidPaint(this);

    
    GdkRegion* dirtyArea = gdk_window_get_update_area(mGdkWindow);
    if (dirtyArea) {
        gdk_window_invalidate_region(mGdkWindow, dirtyArea, PR_FALSE);
        gdk_region_destroy(dirtyArea);
        gdk_window_process_updates(mGdkWindow, PR_FALSE);
    }

    
    return TRUE;
}

gboolean
nsWindow::OnConfigureEvent(GtkWidget *aWidget, GdkEventConfigure *aEvent)
{
    LOG(("configure event [%p] %d %d %d %d\n", (void *)this,
         aEvent->x, aEvent->y, aEvent->width, aEvent->height));

    
    if (mBounds.x == aEvent->x &&
        mBounds.y == aEvent->y)
        return FALSE;

    if (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog) {
        check_for_rollup(aEvent->window, 0, 0, PR_FALSE, PR_TRUE);
    }

    
    
    
    nsIntPoint pnt(aEvent->x, aEvent->y);
    if (mIsTopLevel) {
        
        mBounds.MoveTo(WidgetToScreenOffset());
        pnt = mBounds.TopLeft();
    }

    nsGUIEvent event(PR_TRUE, NS_MOVE, this);

    event.refPoint = pnt;

    
    
    nsEventStatus status;
    DispatchEvent(&event, status);

    return FALSE;
}

void
nsWindow::OnContainerUnrealize(GtkWidget *aWidget)
{
    
    
    

    NS_ASSERTION(mContainer == MOZ_CONTAINER(aWidget),
                 "unexpected \"unrealize\" signal");

    if (mGdkWindow) {
        DestroyChildWindows();

        g_object_set_data(G_OBJECT(mGdkWindow), "nsWindow", NULL);
        mGdkWindow = NULL;
    }
}

void
nsWindow::OnSizeAllocate(GtkWidget *aWidget, GtkAllocation *aAllocation)
{
    LOG(("size_allocate [%p] %d %d %d %d\n",
         (void *)this, aAllocation->x, aAllocation->y,
         aAllocation->width, aAllocation->height));

    nsIntRect rect(aAllocation->x, aAllocation->y,
                   aAllocation->width, aAllocation->height);

    ResizeTransparencyBitmap(rect.width, rect.height);

    mBounds.width = rect.width;
    mBounds.height = rect.height;

    if (!mGdkWindow)
        return;

    if (mTransparencyBitmap) {
      ApplyTransparencyBitmap();
    }

    nsEventStatus status;
    DispatchResizeEvent (rect, status);
}

void
nsWindow::OnDeleteEvent(GtkWidget *aWidget, GdkEventAny *aEvent)
{
    nsGUIEvent event(PR_TRUE, NS_XUL_CLOSE, this);

    event.refPoint.x = 0;
    event.refPoint.y = 0;

    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::OnEnterNotifyEvent(GtkWidget *aWidget, GdkEventCrossing *aEvent)
{
    
    
    
    
    
    if (aEvent->subwindow != NULL)
        return;

    
    
    DispatchMissedButtonReleases(aEvent);

    if (is_parent_ungrab_enter(aEvent))
        return;

    nsMouseEvent event(PR_TRUE, NS_MOUSE_ENTER, this, nsMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->x);
    event.refPoint.y = nscoord(aEvent->y);

    event.time = aEvent->time;

    LOG(("OnEnterNotify: %p\n", (void *)this));

    nsEventStatus status;
    DispatchEvent(&event, status);
}


static PRBool
is_top_level_mouse_exit(GdkWindow* aWindow, GdkEventCrossing *aEvent)
{
    gint x = gint(aEvent->x_root);
    gint y = gint(aEvent->y_root);
    GdkDisplay* display = gdk_drawable_get_display(aWindow);
    GdkWindow* winAtPt = gdk_display_get_window_at_pointer(display, &x, &y);
    if (!winAtPt)
        return PR_TRUE;
    GdkWindow* topLevelAtPt = gdk_window_get_toplevel(winAtPt);
    GdkWindow* topLevelWidget = gdk_window_get_toplevel(aWindow);
    return topLevelAtPt != topLevelWidget;
}

void
nsWindow::OnLeaveNotifyEvent(GtkWidget *aWidget, GdkEventCrossing *aEvent)
{
    
    
    
    
    
    
    
    
    if (aEvent->subwindow != NULL)
        return;

    nsMouseEvent event(PR_TRUE, NS_MOUSE_EXIT, this, nsMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->x);
    event.refPoint.y = nscoord(aEvent->y);

    event.time = aEvent->time;

    event.exit = is_top_level_mouse_exit(mGdkWindow, aEvent)
        ? nsMouseEvent::eTopLevel : nsMouseEvent::eChild;

    LOG(("OnLeaveNotify: %p\n", (void *)this));

    nsEventStatus status;
    DispatchEvent(&event, status);
}

#ifdef MOZ_DFB
void
nsWindow::OnMotionNotifyEvent(GtkWidget *aWidget, GdkEventMotion *aEvent)
{
    int cursorX = (int) aEvent->x_root;
    int cursorY = (int) aEvent->y_root;

    D_DEBUG_AT( ns_Window, "%s( %4d,%4d - [%d] )\n", __FUNCTION__, cursorX, cursorY, mDFBCursorCount );

    D_ASSUME( mDFBLayer != NULL );

    if (mDFBLayer)
         mDFBLayer->GetCursorPosition( mDFBLayer, &cursorX, &cursorY );

    mDFBCursorCount++;

#if D_DEBUG_ENABLED
    if (cursorX != (int) aEvent->x_root || cursorY != (int) aEvent->y_root)
         D_DEBUG_AT( ns_Window, "  -> forward to %4d,%4d\n", cursorX, cursorY );
#endif

    if (cursorX == mDFBCursorX && cursorY == mDFBCursorY) {
         D_DEBUG_AT( ns_Window, "  -> dropping %4d,%4d\n", cursorX, cursorY );

         
         return;
    }

    mDFBCursorX = cursorX;
    mDFBCursorY = cursorY;


    
    
    sIsDraggingOutOf = PR_FALSE;

    nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, this, nsMouseEvent::eReal);

    
    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    
    
    if (pressure)
      mLastMotionPressure = pressure;
    event.pressure = mLastMotionPressure;

    event.refPoint = nsIntPoint(cursorX, cursorY) - WidgetToScreenOffset();

    event.isShift   = (aEvent->state & GDK_SHIFT_MASK)
        ? PR_TRUE : PR_FALSE;
    event.isControl = (aEvent->state & GDK_CONTROL_MASK)
        ? PR_TRUE : PR_FALSE;
    event.isAlt     = (aEvent->state & GDK_MOD1_MASK)
        ? PR_TRUE : PR_FALSE;

    event.time = aEvent->time;

    nsEventStatus status;
    DispatchEvent(&event, status);
}
#else
void
nsWindow::OnMotionNotifyEvent(GtkWidget *aWidget, GdkEventMotion *aEvent)
{
    
    
    sIsDraggingOutOf = PR_FALSE;

    
    
    
    PRPackedBool synthEvent = PR_FALSE;
#ifdef MOZ_X11
    XEvent xevent;

    while (XPending (GDK_WINDOW_XDISPLAY(aEvent->window))) {
        XEvent peeked;
        XPeekEvent (GDK_WINDOW_XDISPLAY(aEvent->window), &peeked);
        if (peeked.xany.window != GDK_WINDOW_XWINDOW(aEvent->window)
            || peeked.type != MotionNotify)
            break;

        synthEvent = PR_TRUE;
        XNextEvent (GDK_WINDOW_XDISPLAY(aEvent->window), &xevent);
    }

    
    if (gPluginFocusWindow && gPluginFocusWindow != this) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gPluginFocusWindow;
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }
#endif 

    nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, this, nsMouseEvent::eReal);

    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    
    
    if (pressure)
      mLastMotionPressure = pressure;
    event.pressure = mLastMotionPressure;

    if (synthEvent) {
#ifdef MOZ_X11
        event.refPoint.x = nscoord(xevent.xmotion.x);
        event.refPoint.y = nscoord(xevent.xmotion.y);

        event.isShift   = (xevent.xmotion.state & GDK_SHIFT_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isControl = (xevent.xmotion.state & GDK_CONTROL_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isAlt     = (xevent.xmotion.state & GDK_MOD1_MASK)
            ? PR_TRUE : PR_FALSE;

        event.time = xevent.xmotion.time;
#else
        event.refPoint.x = nscoord(aEvent->x);
        event.refPoint.y = nscoord(aEvent->y);

        event.isShift   = (aEvent->state & GDK_SHIFT_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isControl = (aEvent->state & GDK_CONTROL_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isAlt     = (aEvent->state & GDK_MOD1_MASK)
            ? PR_TRUE : PR_FALSE;

        event.time = aEvent->time;
#endif 
    }
    else {
        
        if (aEvent->window == mGdkWindow) {
            event.refPoint.x = nscoord(aEvent->x);
            event.refPoint.y = nscoord(aEvent->y);
        } else {
            nsIntPoint point(NSToIntFloor(aEvent->x_root), NSToIntFloor(aEvent->y_root));
            event.refPoint = point - WidgetToScreenOffset();
        }

        event.isShift   = (aEvent->state & GDK_SHIFT_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isControl = (aEvent->state & GDK_CONTROL_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isAlt     = (aEvent->state & GDK_MOD1_MASK)
            ? PR_TRUE : PR_FALSE;

        event.time = aEvent->time;
    }

    nsEventStatus status;
    DispatchEvent(&event, status);
}
#endif








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
            PRInt16 buttonType;
            switch (buttonMask) {
            case GDK_BUTTON1_MASK:
                buttonType = nsMouseEvent::eLeftButton;
                break;
            case GDK_BUTTON2_MASK:
                buttonType = nsMouseEvent::eMiddleButton;
                break;
            default:
                NS_ASSERTION(buttonMask == GDK_BUTTON3_MASK,
                             "Unexpected button mask");
                buttonType = nsMouseEvent::eRightButton;
            }

            LOG(("Synthesized button %u release on %p\n",
                 guint(buttonType + 1), (void *)this));

            
            
            
            
            nsMouseEvent synthEvent(PR_TRUE, NS_MOUSE_BUTTON_UP, this,
                                    nsMouseEvent::eSynthesized);
            synthEvent.button = buttonType;
            nsEventStatus status;
            DispatchEvent(&synthEvent, status);

            sLastButtonReleaseTime = aGdkEvent->time;
        }
    }
}

void
nsWindow::InitButtonEvent(nsMouseEvent &aEvent,
                          GdkEventButton *aGdkEvent)
{
    
    if (aGdkEvent->window == mGdkWindow) {
        aEvent.refPoint.x = nscoord(aGdkEvent->x);
        aEvent.refPoint.y = nscoord(aGdkEvent->y);
    } else {
        nsIntPoint point(NSToIntFloor(aGdkEvent->x_root), NSToIntFloor(aGdkEvent->y_root));
        aEvent.refPoint = point - WidgetToScreenOffset();
    }

    aEvent.isShift   = (aGdkEvent->state & GDK_SHIFT_MASK) != 0;
    aEvent.isControl = (aGdkEvent->state & GDK_CONTROL_MASK) != 0;
    aEvent.isAlt     = (aGdkEvent->state & GDK_MOD1_MASK) != 0;
    aEvent.isMeta    = (aGdkEvent->state & GDK_MOD4_MASK) != 0;

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
nsWindow::OnButtonPressEvent(GtkWidget *aWidget, GdkEventButton *aEvent)
{
    LOG(("Button %u press on %p\n", aEvent->button, (void *)this));

    nsEventStatus status;

    
    
    
    
    
    
    GdkEvent *peekedEvent = gdk_event_peek();
    if (peekedEvent) {
        GdkEventType type = peekedEvent->any.type;
        gdk_event_free(peekedEvent);
        if (type == GDK_2BUTTON_PRESS || type == GDK_3BUTTON_PRESS)
            return;
    }

    
    sLastButtonPressTime = aEvent->time;
    sLastButtonReleaseTime = 0;

    nsWindow *containerWindow = GetContainerWindow();
    if (!gFocusWindow && containerWindow) {
        containerWindow->DispatchActivateEvent();
    }

    
    PRBool rolledUp = check_for_rollup(aEvent->window, aEvent->x_root,
                                       aEvent->y_root, PR_FALSE, PR_FALSE);
    if (gConsumeRollupEvent && rolledUp)
        return;

    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    mLastMotionPressure = pressure;

    PRUint16 domButton;
    switch (aEvent->button) {
    case 1:
        domButton = nsMouseEvent::eLeftButton;
        break;
    case 2:
        domButton = nsMouseEvent::eMiddleButton;
        break;
    case 3:
        domButton = nsMouseEvent::eRightButton;
        break;
    
    case 6:
    case 7:
        {
            nsMouseScrollEvent event(PR_TRUE, NS_MOUSE_SCROLL, this);
            event.pressure = mLastMotionPressure;
            event.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
            event.refPoint.x = nscoord(aEvent->x);
            event.refPoint.y = nscoord(aEvent->y);
            
            event.delta = (aEvent->button == 6) ? -2 : 2;

            event.isShift   = (aEvent->state & GDK_SHIFT_MASK) != 0;
            event.isControl = (aEvent->state & GDK_CONTROL_MASK) != 0;
            event.isAlt     = (aEvent->state & GDK_MOD1_MASK) != 0;
            event.isMeta    = (aEvent->state & GDK_MOD4_MASK) != 0;

            event.time = aEvent->time;

            nsEventStatus status;
            DispatchEvent(&event, status);
            return;
        }
    
    case 8:
        DispatchCommandEvent(nsWidgetAtoms::Back);
        return;
    case 9:
        DispatchCommandEvent(nsWidgetAtoms::Forward);
        return;
    default:
        return;
    }

    gButtonState |= ButtonMaskFromGDKButton(aEvent->button);

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_DOWN, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent);
    event.pressure = mLastMotionPressure;

    DispatchEvent(&event, status);

    
    if (domButton == nsMouseEvent::eRightButton &&
        NS_LIKELY(!mIsDestroyed)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal);
        InitButtonEvent(contextMenuEvent, aEvent);
        contextMenuEvent.pressure = mLastMotionPressure;
        DispatchEvent(&contextMenuEvent, status);
    }
}

void
nsWindow::OnButtonReleaseEvent(GtkWidget *aWidget, GdkEventButton *aEvent)
{
    LOG(("Button %u release on %p\n", aEvent->button, (void *)this));

    PRUint16 domButton;
    sLastButtonReleaseTime = aEvent->time;

    switch (aEvent->button) {
    case 1:
        domButton = nsMouseEvent::eLeftButton;
        break;
    case 2:
        domButton = nsMouseEvent::eMiddleButton;
        break;
    case 3:
        domButton = nsMouseEvent::eRightButton;
        break;
    default:
        return;
    }

    gButtonState &= ~ButtonMaskFromGDKButton(aEvent->button);

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_UP, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent);
    gdouble pressure = 0;
    gdk_event_get_axis ((GdkEvent*)aEvent, GDK_AXIS_PRESSURE, &pressure);
    event.pressure = pressure ? pressure : mLastMotionPressure;

    nsEventStatus status;
    DispatchEvent(&event, status);
    mLastMotionPressure = pressure;
}

void
nsWindow::OnContainerFocusInEvent(GtkWidget *aWidget, GdkEventFocus *aEvent)
{
    NS_ASSERTION(mWindowType != eWindowType_popup,
                 "Unexpected focus on a popup window");

    LOGFOCUS(("OnContainerFocusInEvent [%p]\n", (void *)this));
    if (!mEnabled) {
        LOGFOCUS(("Container focus is blocked [%p]\n", (void *)this));
        return;
    }

    
    GtkWidget* top_window = nsnull;
    GetToplevelWidget(&top_window);
    if (top_window && (GTK_WIDGET_VISIBLE(top_window)))
        SetUrgencyHint(top_window, PR_FALSE);

    
    
    if (gBlockActivateEvent) {
        LOGFOCUS(("NS_ACTIVATE event is blocked [%p]\n", (void *)this));
        return;
    }

    
    
    
    
    
    gFocusWindow = this;

    DispatchActivateEvent();

    LOGFOCUS(("Events sent from focus in event [%p]\n", (void *)this));
}

void
nsWindow::OnContainerFocusOutEvent(GtkWidget *aWidget, GdkEventFocus *aEvent)
{
    LOGFOCUS(("OnContainerFocusOutEvent [%p]\n", (void *)this));

    if (mWindowType == eWindowType_toplevel || mWindowType == eWindowType_dialog) {
        nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
        nsCOMPtr<nsIDragSession> dragSession;
        dragService->GetCurrentSession(getter_AddRefs(dragSession));

        
        
        
        PRBool shouldRollup = !dragSession;
        if (!shouldRollup) {
            
            nsCOMPtr<nsIDOMNode> sourceNode;
            dragSession->GetSourceNode(getter_AddRefs(sourceNode));
            shouldRollup = (sourceNode == nsnull);
        }

        if (shouldRollup) {
            check_for_rollup(aEvent->window, 0, 0, PR_FALSE, PR_TRUE);
        }
    }

#ifdef MOZ_X11
    
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
        gFocusWindow = nsnull;
    }

    DispatchDeactivateEvent();

    LOGFOCUS(("Done with container focus out [%p]\n", (void *)this));
}

PRBool
nsWindow::DispatchCommandEvent(nsIAtom* aCommand)
{
    nsEventStatus status;
    nsCommandEvent event(PR_TRUE, nsWidgetAtoms::onAppCommand, aCommand, this);
    DispatchEvent(&event, status);
    return TRUE;
}

static PRUint32
GetCharCodeFor(const GdkEventKey *aEvent, guint aShiftState,
               gint aGroup)
{
    guint keyval;
    if (gdk_keymap_translate_keyboard_state(NULL, aEvent->hardware_keycode,
                                            GdkModifierType(aShiftState),
                                            aGroup,
                                            &keyval, NULL, NULL, NULL)) {
        GdkEventKey tmpEvent = *aEvent;
        tmpEvent.state = guint(aShiftState);
        tmpEvent.keyval = keyval;
        tmpEvent.group = aGroup;
        return nsConvertCharCodeToUnicode(&tmpEvent);
    }
    return 0;
}

static gint
GetKeyLevel(GdkEventKey *aEvent)
{
    gint level;
    if (!gdk_keymap_translate_keyboard_state(NULL,
                                             aEvent->hardware_keycode,
                                             GdkModifierType(aEvent->state),
                                             aEvent->group,
                                             NULL, NULL, &level, NULL))
        return -1;
    return level;
}

static PRBool
IsBasicLatinLetterOrNumeral(PRUint32 aChar)
{
    return (aChar >= 'a' && aChar <= 'z') ||
           (aChar >= 'A' && aChar <= 'Z') ||
           (aChar >= '0' && aChar <= '9');
}

static PRBool
IsCtrlAltTab(GdkEventKey *aEvent)
{
    return aEvent->keyval == GDK_Tab &&
        aEvent->state & GDK_CONTROL_MASK && aEvent->state & GDK_MOD1_MASK;
}

PRBool
nsWindow::DispatchKeyDownEvent(GdkEventKey *aEvent, PRBool *aCancelled)
{
    NS_PRECONDITION(aCancelled, "aCancelled must not be null");

    *aCancelled = PR_FALSE;

    if (IsCtrlAltTab(aEvent)) {
        return PR_FALSE;
    }

    
    nsEventStatus status;
    nsKeyEvent downEvent(PR_TRUE, NS_KEY_DOWN, this);
    InitKeyEvent(downEvent, aEvent);
    DispatchEvent(&downEvent, status);
    *aCancelled = (status == nsEventStatus_eConsumeNoDefault);
    return PR_TRUE;
}

gboolean
nsWindow::OnKeyPressEvent(GtkWidget *aWidget, GdkEventKey *aEvent)
{
    LOGFOCUS(("OnKeyPressEvent [%p]\n", (void *)this));

    
    
    PRBool IMEWasEnabled = PR_FALSE;
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

    
    
    
    
    

    PRBool isKeyDownCancelled = PR_FALSE;
    if (DispatchKeyDownEvent(aEvent, &isKeyDownCancelled) &&
        NS_UNLIKELY(mIsDestroyed)) {
        return TRUE;
    }

    
    
    
    if (!IMEWasEnabled && mIMModule && mIMModule->IsEnabled()) {
        
        
        if (mIMModule->OnKeyEvent(this, aEvent, PR_TRUE)) {
            return TRUE;
        }
    }

    
    
    
    
    if (aEvent->keyval == GDK_Shift_L
        || aEvent->keyval == GDK_Shift_R
        || aEvent->keyval == GDK_Control_L
        || aEvent->keyval == GDK_Control_R
        || aEvent->keyval == GDK_Alt_L
        || aEvent->keyval == GDK_Alt_R
        || aEvent->keyval == GDK_Meta_L
        || aEvent->keyval == GDK_Meta_R) {
        return TRUE;
    }

#ifdef MOZ_X11
#if ! defined AIX 
    
    switch (aEvent->keyval) {
        case XF86XK_Back:
            return DispatchCommandEvent(nsWidgetAtoms::Back);
        case XF86XK_Forward:
            return DispatchCommandEvent(nsWidgetAtoms::Forward);
        case XF86XK_Refresh:
            return DispatchCommandEvent(nsWidgetAtoms::Reload);
        case XF86XK_Stop:
            return DispatchCommandEvent(nsWidgetAtoms::Stop);
        case XF86XK_Search:
            return DispatchCommandEvent(nsWidgetAtoms::Search);
        case XF86XK_Favorites:
            return DispatchCommandEvent(nsWidgetAtoms::Bookmarks);
        case XF86XK_HomePage:
            return DispatchCommandEvent(nsWidgetAtoms::Home);
    }
#endif 
#endif 

    nsKeyEvent event(PR_TRUE, NS_KEY_PRESS, this);
    InitKeyEvent(event, aEvent);
    if (isKeyDownCancelled) {
      
      event.flags |= NS_EVENT_FLAG_NO_DEFAULT;
    }
    event.charCode = nsConvertCharCodeToUnicode(aEvent);
    if (event.charCode) {
        event.keyCode = 0;
        gint level = GetKeyLevel(aEvent);
        if ((event.isControl || event.isAlt || event.isMeta) &&
            (level == 0 || level == 1)) {
            guint baseState =
                aEvent->state & ~(GDK_SHIFT_MASK | GDK_CONTROL_MASK |
                                  GDK_MOD1_MASK | GDK_MOD4_MASK);
            
            
            
            
            nsAlternativeCharCode altCharCodes(0, 0);
            
            altCharCodes.mUnshiftedCharCode =
                GetCharCodeFor(aEvent, baseState, aEvent->group);
            PRBool isLatin = (altCharCodes.mUnshiftedCharCode <= 0xFF);
            
            altCharCodes.mShiftedCharCode =
                GetCharCodeFor(aEvent, baseState | GDK_SHIFT_MASK,
                               aEvent->group);
            isLatin = isLatin && (altCharCodes.mShiftedCharCode <= 0xFF);
            if (altCharCodes.mUnshiftedCharCode ||
                altCharCodes.mShiftedCharCode) {
                event.alternativeCharCodes.AppendElement(altCharCodes);
            }

            if (!isLatin) {
                
                GdkKeymapKey *keys;
                gint count;
                gint minGroup = -1;
                if (gdk_keymap_get_entries_for_keyval(NULL, GDK_a,
                                                      &keys, &count)) {
                    
                    for (gint i = 0; i < count && minGroup != 0; ++i) {
                        if (keys[i].level != 0 && keys[i].level != 1)
                            continue;
                        if (minGroup >= 0 && keys[i].group > minGroup)
                            continue;
                        minGroup = keys[i].group;
                    }
                    g_free(keys);
                }
                if (minGroup >= 0) {
                    PRUint32 unmodifiedCh =
                               event.isShift ? altCharCodes.mShiftedCharCode :
                                               altCharCodes.mUnshiftedCharCode;
                    
                    PRUint32 ch =
                        GetCharCodeFor(aEvent, baseState, minGroup);
                    altCharCodes.mUnshiftedCharCode =
                        IsBasicLatinLetterOrNumeral(ch) ? ch : 0;
                    
                    ch = GetCharCodeFor(aEvent, baseState | GDK_SHIFT_MASK,
                                        minGroup);
                    altCharCodes.mShiftedCharCode =
                        IsBasicLatinLetterOrNumeral(ch) ? ch : 0;
                    if (altCharCodes.mUnshiftedCharCode ||
                        altCharCodes.mShiftedCharCode) {
                        event.alternativeCharCodes.AppendElement(altCharCodes);
                    }
                    
                    
                    
                    
                    
                    ch = event.isShift ? altCharCodes.mShiftedCharCode :
                                         altCharCodes.mUnshiftedCharCode;
                    if (ch && !(event.isAlt || event.isMeta) &&
                        event.charCode == unmodifiedCh) {
                        event.charCode = ch;
                    }
                }
            }
        }
    }

    
    
    if (is_context_menu_key(event)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal,
                                      nsMouseEvent::eContextMenuKey);
        key_event_to_context_menu_event(contextMenuEvent, aEvent);
        DispatchEvent(&contextMenuEvent, status);
    }
    else {
        
        
        if (IS_IN_BMP(event.charCode)) {
            DispatchEvent(&event, status);
        }
        else {
            nsTextEvent textEvent(PR_TRUE, NS_TEXT_TEXT, this);
            PRUnichar textString[3];
            textString[0] = H_SURROGATE(event.charCode);
            textString[1] = L_SURROGATE(event.charCode);
            textString[2] = 0;
            textEvent.theText = textString;
            textEvent.time = event.time;
            DispatchEvent(&textEvent, status);
        }
    }

    
    if (status == nsEventStatus_eConsumeNoDefault) {
        return TRUE;
    }

    return FALSE;
}

gboolean
nsWindow::OnKeyReleaseEvent(GtkWidget *aWidget, GdkEventKey *aEvent)
{
    LOGFOCUS(("OnKeyReleaseEvent [%p]\n", (void *)this));

    if (mIMModule && mIMModule->OnKeyEvent(this, aEvent)) {
        return TRUE;
    }

    
    nsKeyEvent event(PR_TRUE, NS_KEY_UP, this);
    InitKeyEvent(event, aEvent);

    nsEventStatus status;
    DispatchEvent(&event, status);

    
    if (status == nsEventStatus_eConsumeNoDefault) {
        return TRUE;
    }

    return FALSE;
}

void
nsWindow::OnScrollEvent(GtkWidget *aWidget, GdkEventScroll *aEvent)
{
    
    PRBool rolledUp =  check_for_rollup(aEvent->window, aEvent->x_root,
                                        aEvent->y_root, PR_TRUE, PR_FALSE);
    if (gConsumeRollupEvent && rolledUp)
        return;

    nsMouseScrollEvent event(PR_TRUE, NS_MOUSE_SCROLL, this);
    switch (aEvent->direction) {
    case GDK_SCROLL_UP:
        event.scrollFlags = nsMouseScrollEvent::kIsVertical;
        event.delta = -3;
        break;
    case GDK_SCROLL_DOWN:
        event.scrollFlags = nsMouseScrollEvent::kIsVertical;
        event.delta = 3;
        break;
    case GDK_SCROLL_LEFT:
        event.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
        event.delta = -1;
        break;
    case GDK_SCROLL_RIGHT:
        event.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
        event.delta = 1;
        break;
    }

    if (aEvent->window == mGdkWindow) {
        
        event.refPoint.x = nscoord(aEvent->x);
        event.refPoint.y = nscoord(aEvent->y);
    } else {
        
        
        
        nsIntPoint point(NSToIntFloor(aEvent->x_root), NSToIntFloor(aEvent->y_root));
        event.refPoint = point - WidgetToScreenOffset();
    }

    event.isShift   = (aEvent->state & GDK_SHIFT_MASK) != 0;
    event.isControl = (aEvent->state & GDK_CONTROL_MASK) != 0;
    event.isAlt     = (aEvent->state & GDK_MOD1_MASK) != 0;
    event.isMeta    = (aEvent->state & GDK_MOD4_MASK) != 0;

    event.time = aEvent->time;

    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::OnVisibilityNotifyEvent(GtkWidget *aWidget,
                                  GdkEventVisibility *aEvent)
{
    LOGDRAW(("Visibility event %i on [%p] %p\n",
             aEvent->state, this, aEvent->window));

    if (!mGdkWindow)
        return;

    switch (aEvent->state) {
    case GDK_VISIBILITY_UNOBSCURED:
    case GDK_VISIBILITY_PARTIAL:
        if (mIsFullyObscured && mHasMappedToplevel) {
            
            
            gdk_window_invalidate_rect(mGdkWindow, NULL, FALSE);
        }

        mIsFullyObscured = PR_FALSE;

        
        
        
        
        
        
        if (!nsGtkIMModule::IsVirtualKeyboardOpened()) {
            
            EnsureGrabs();
        }
        break;
    default: 
        mIsFullyObscured = PR_TRUE;
        break;
    }
}

void
nsWindow::OnWindowStateEvent(GtkWidget *aWidget, GdkEventWindowState *aEvent)
{
    LOG(("nsWindow::OnWindowStateEvent [%p] changed %d new_window_state %d\n",
         (void *)this, aEvent->changed_mask, aEvent->new_window_state));

    if (IS_MOZ_CONTAINER(aWidget)) {
        
        
        
        
        
        
        
        
        
        
        PRBool mapped =
            !(aEvent->new_window_state &
              (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN));
        if (mHasMappedToplevel != mapped) {
            SetHasMappedToplevel(mapped);
        }
        return;
    }
    

    nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);

    
    
    if ((aEvent->changed_mask
         & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_MAXIMIZED)) == 0) {
        return;
    }

    if (aEvent->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
        LOG(("\tIconified\n"));
        event.mSizeMode = nsSizeMode_Minimized;
        mSizeState = nsSizeMode_Minimized;
#ifdef ACCESSIBILITY
        DispatchMinimizeEventAccessible();
#endif 
    }
    else if (aEvent->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        LOG(("\tMaximized\n"));
        event.mSizeMode = nsSizeMode_Maximized;
        mSizeState = nsSizeMode_Maximized;
#ifdef ACCESSIBILITY
        DispatchMaximizeEventAccessible();
#endif 
    }
    else if (aEvent->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
        LOG(("\tFullscreen\n"));
        event.mSizeMode = nsSizeMode_Fullscreen;
        mSizeState = nsSizeMode_Fullscreen;
    }
    else {
        LOG(("\tNormal\n"));
        event.mSizeMode = nsSizeMode_Normal;
        mSizeState = nsSizeMode_Normal;
#ifdef ACCESSIBILITY
        DispatchRestoreEventAccessible();
#endif 
    }

    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::ThemeChanged()
{
    nsGUIEvent event(PR_TRUE, NS_THEMECHANGED, this);
    nsEventStatus status = nsEventStatus_eIgnore;
    DispatchEvent(&event, status);

    if (!mGdkWindow || NS_UNLIKELY(mIsDestroyed))
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
nsWindow::CheckNeedDragLeaveEnter(nsWindow* aInnerMostWidget,
                                  nsIDragService* aDragService,
                                  GdkDragContext *aDragContext,
                                  nscoord aX, nscoord aY)
{
    
    if (sLastDragMotionWindow) {
        
        if (sLastDragMotionWindow == aInnerMostWidget) {
            UpdateDragStatus(aDragContext, aDragService);
            return;
        }

        
        nsRefPtr<nsWindow> kungFuDeathGrip = sLastDragMotionWindow;
        sLastDragMotionWindow->OnDragLeave();
    }

    
    aDragService->StartDragSession();

    
    UpdateDragStatus(aDragContext, aDragService);
    aInnerMostWidget->OnDragEnter(aX, aY);

    
    sLastDragMotionWindow = aInnerMostWidget;
}

gboolean
nsWindow::OnDragMotionEvent(GtkWidget *aWidget,
                            GdkDragContext *aDragContext,
                            gint aX,
                            gint aY,
                            guint aTime,
                            gpointer aData)
{
    LOGDRAG(("nsWindow::OnDragMotionSignal\n"));

    if (sLastButtonReleaseTime) {
      
      
      GtkWidget *widget = gtk_grab_get_current();
      GdkEvent event;
      gboolean retval;
      memset(&event, 0, sizeof(event));
      event.type = GDK_BUTTON_RELEASE;
      event.button.time = sLastButtonReleaseTime;
      event.button.button = 1;
      sLastButtonReleaseTime = 0;
      if (widget) {
        g_signal_emit_by_name(widget, "button_release_event", &event, &retval);
        return TRUE;
      }
    }

    sIsDraggingOutOf = PR_FALSE;

    
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
    nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

    
    
    nscoord retx = 0;
    nscoord rety = 0;

    GdkWindow *innerWindow = get_inner_gdk_window(aWidget->window, aX, aY,
                                                  &retx, &rety);
    nsRefPtr<nsWindow> innerMostWidget = get_window_for_gdk_window(innerWindow);

    if (!innerMostWidget)
        innerMostWidget = this;

    
    dragSessionGTK->TargetSetLastContext(aWidget, aDragContext, aTime);

    
    
    if (mDragLeaveTimer) {
        mDragLeaveTimer->Cancel();
        mDragLeaveTimer = nsnull;
    }

    CheckNeedDragLeaveEnter(innerMostWidget, dragService, aDragContext, retx, rety);

    
    dragSessionGTK->TargetStartDragMotion();

    dragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);

    nsDragEvent event(PR_TRUE, NS_DRAGDROP_OVER, innerMostWidget);

    InitDragEvent(event);

    event.refPoint.x = retx;
    event.refPoint.y = rety;
    event.time = aTime;

    nsEventStatus status;
    innerMostWidget->DispatchEvent(&event, status);

    
    dragSessionGTK->TargetEndDragMotion(aWidget, aDragContext, aTime);

    
    dragSessionGTK->TargetSetLastContext(0, 0, 0);

    return TRUE;
}

void
nsWindow::OnDragLeaveEvent(GtkWidget *aWidget,
                           GdkDragContext *aDragContext,
                           guint aTime,
                           gpointer aData)
{
    

    LOGDRAG(("nsWindow::OnDragLeaveSignal(%p)\n", (void*)this));

    sIsDraggingOutOf = PR_TRUE;

    if (mDragLeaveTimer) {
        return;
    }

    
    
    
    mDragLeaveTimer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ASSERTION(mDragLeaveTimer, "Failed to create drag leave timer!");
    
    mDragLeaveTimer->InitWithFuncCallback(DragLeaveTimerCallback,
                                          (void *)this,
                                          20, nsITimer::TYPE_ONE_SHOT);
}

gboolean
nsWindow::OnDragDropEvent(GtkWidget *aWidget,
                          GdkDragContext *aDragContext,
                          gint aX,
                          gint aY,
                          guint aTime,
                          gpointer *aData)

{
    LOGDRAG(("nsWindow::OnDragDropSignal\n"));

    
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
    nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

    nscoord retx = 0;
    nscoord rety = 0;

    GdkWindow *innerWindow = get_inner_gdk_window(aWidget->window, aX, aY,
                                                  &retx, &rety);
    nsRefPtr<nsWindow> innerMostWidget = get_window_for_gdk_window(innerWindow);

    if (!innerMostWidget)
        innerMostWidget = this;

    
    dragSessionGTK->TargetSetLastContext(aWidget, aDragContext, aTime);

    
    
    if (mDragLeaveTimer) {
        mDragLeaveTimer->Cancel();
        mDragLeaveTimer = nsnull;
    }

    CheckNeedDragLeaveEnter(innerMostWidget, dragService, aDragContext, retx, rety);

    
    
    

    nsDragEvent event(PR_TRUE, NS_DRAGDROP_OVER, innerMostWidget);

    InitDragEvent(event);

    event.refPoint.x = retx;
    event.refPoint.y = rety;
    event.time = aTime;

    nsEventStatus status;
    innerMostWidget->DispatchEvent(&event, status);

    
    
    
    if (!innerMostWidget->mIsDestroyed) {
        nsDragEvent event(PR_TRUE, NS_DRAGDROP_DROP, innerMostWidget);
        event.refPoint.x = retx;
        event.refPoint.y = rety;

        nsEventStatus status = nsEventStatus_eIgnore;
        innerMostWidget->DispatchEvent(&event, status);
    }

    

    gdk_drop_finish(aDragContext, TRUE, aTime);

    
    
    
    
    dragSessionGTK->TargetSetLastContext(0, 0, 0);

    
    sLastDragMotionWindow = 0;

    
    
    gint x, y;
    GdkDisplay* display = gdk_display_get_default();
    if (display) {
      
      gdk_display_get_pointer(display, NULL, &x, &y, NULL);
      ((nsDragService *)dragService.get())->SetDragEndPoint(nsIntPoint(x, y));
    }
    dragService->EndDragSession(PR_TRUE);

    return TRUE;
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

    
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
    nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

    dragSessionGTK->TargetDataReceived(aWidget, aDragContext, aX, aY,
                                       aSelectionData, aInfo, aTime);
}

void
nsWindow::OnDragLeave(void)
{
    LOGDRAG(("nsWindow::OnDragLeave(%p)\n", (void*)this));

    nsDragEvent event(PR_TRUE, NS_DRAGDROP_EXIT, this);

    nsEventStatus status;
    DispatchEvent(&event, status);

    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);

    if (dragService) {
        nsCOMPtr<nsIDragSession> currentDragSession;
        dragService->GetCurrentSession(getter_AddRefs(currentDragSession));

        if (currentDragSession) {
            nsCOMPtr<nsIDOMNode> sourceNode;
            currentDragSession->GetSourceNode(getter_AddRefs(sourceNode));

            if (!sourceNode) {
                
                
                
                
                dragService->EndDragSession(PR_FALSE);
            }
        }
    }
}

void
nsWindow::OnDragEnter(nscoord aX, nscoord aY)
{
    

    LOGDRAG(("nsWindow::OnDragEnter(%p)\n", (void*)this));

    nsDragEvent event(PR_TRUE, NS_DRAGDROP_ENTER, this);

    event.refPoint.x = aX;
    event.refPoint.y = aY;

    nsEventStatus status;
    DispatchEvent(&event, status);
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

static GdkWindow *
CreateGdkWindow(GdkWindow *parent, GtkWidget *widget)
{
    GdkWindowAttr attributes;
    gint          attributes_mask = GDK_WA_VISUAL | GDK_WA_COLORMAP;

    attributes.event_mask = (GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK |
                             GDK_VISIBILITY_NOTIFY_MASK |
                             GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                             GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
#ifdef HAVE_GTK_MOTION_HINTS
                             GDK_POINTER_MOTION_HINT_MASK |
#endif
                             GDK_POINTER_MOTION_MASK);

    attributes.width = 1;
    attributes.height = 1;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);
    attributes.window_type = GDK_WINDOW_CHILD;

    GdkWindow *window = gdk_window_new(parent, &attributes, attributes_mask);
    gdk_window_set_user_data(window, widget);

    

    gdk_window_set_back_pixmap(window, NULL, FALSE);

    return window;
}

nsresult
nsWindow::Create(nsIWidget        *aParent,
                 nsNativeWidget    aNativeParent,
                 const nsIntRect  &aRect,
                 EVENT_CALLBACK    aHandleEventFunction,
                 nsDeviceContext *aContext,
                 nsIAppShell      *aAppShell,
                 nsIToolkit       *aToolkit,
                 nsWidgetInitData *aInitData)
{
    
    
    nsIWidget *baseParent = aInitData &&
        (aInitData->mWindowType == eWindowType_dialog ||
         aInitData->mWindowType == eWindowType_toplevel ||
         aInitData->mWindowType == eWindowType_invisible) ?
        nsnull : aParent;

    NS_ASSERTION(!mWindowGroup, "already have window group (leaking it)");

    
    BaseCreate(baseParent, aRect, aHandleEventFunction, aContext,
               aAppShell, aToolkit, aInitData);

    
    PRBool listenForResizes = PR_FALSE;;
    if (aNativeParent || (aInitData && aInitData->mListenForResizes))
        listenForResizes = PR_TRUE;

    
    CommonCreate(aParent, listenForResizes);

    
    mBounds = aRect;
    if (mWindowType != eWindowType_child &&
        mWindowType != eWindowType_plugin) {
        
        
        
        
        
        
        mNeedsResize = PR_TRUE;
    }

    
    GtkWidget      *parentMozContainer = nsnull;
    GtkContainer   *parentGtkContainer = nsnull;
    GdkWindow      *parentGdkWindow = nsnull;
    GtkWindow      *topLevelParent = nsnull;

    if (aParent)
        parentGdkWindow = GDK_WINDOW(aParent->GetNativeData(NS_NATIVE_WINDOW));
    else if (aNativeParent && GDK_IS_WINDOW(aNativeParent))
        parentGdkWindow = GDK_WINDOW(aNativeParent);
    else if (aNativeParent && GTK_IS_CONTAINER(aNativeParent))
        parentGtkContainer = GTK_CONTAINER(aNativeParent);

    if (parentGdkWindow) {
        
        parentMozContainer = get_gtk_widget_for_gdk_window(parentGdkWindow);

        if (!IS_MOZ_CONTAINER(parentMozContainer))
            return NS_ERROR_FAILURE;

        
        
        topLevelParent =
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(parentMozContainer)));
    }

    
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
            mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            SetDefaultIcon();
            gtk_window_set_wmclass(GTK_WINDOW(mShell), "Dialog", cBrand.get());
            gtk_window_set_type_hint(GTK_WINDOW(mShell),
                                     GDK_WINDOW_TYPE_HINT_DIALOG);
            gtk_window_set_transient_for(GTK_WINDOW(mShell),
                                         topLevelParent);
            mTransientParent = topLevelParent;
            
            if (!topLevelParent) {
                gtk_widget_realize(mShell);
                GdkWindow* dialoglead = mShell->window;
                gdk_window_set_group(dialoglead, dialoglead);
            }
            if (parentGdkWindow) {
                nsWindow *parentnsWindow =
                    get_window_for_gdk_window(parentGdkWindow);
                NS_ASSERTION(parentnsWindow,
                             "no nsWindow for parentGdkWindow!");
                if (parentnsWindow && parentnsWindow->mWindowGroup) {
                    gtk_window_group_add_window(parentnsWindow->mWindowGroup,
                                                GTK_WINDOW(mShell));
                    
                    mWindowGroup = parentnsWindow->mWindowGroup;
                    g_object_ref(mWindowGroup);
                    LOG(("adding window %p to group %p\n",
                         (void *)mShell, (void *)mWindowGroup));
                }
            }
        }
        else if (mWindowType == eWindowType_popup) {
            
            
            if (!aInitData->mNoAutoHide) {
                
                
                
                mShell = gtk_window_new(GTK_WINDOW_POPUP);
                gtk_window_set_wmclass(GTK_WINDOW(mShell), "Popup", cBrand.get());
            } else {
                
                
                mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
                gtk_window_set_wmclass(GTK_WINDOW(mShell), "Popup", cBrand.get());
                
                
                if (mBorderStyle == eBorderStyle_default) {
                  gtk_window_set_decorated(GTK_WINDOW(mShell), FALSE);
                }
                else {
                  PRBool decorate = mBorderStyle & eBorderStyle_title;
                  gtk_window_set_decorated(GTK_WINDOW(mShell), decorate);
                  if (decorate) {
                    gtk_window_set_deletable(GTK_WINDOW(mShell), mBorderStyle & eBorderStyle_close);
                  }
                }
                gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mShell), TRUE);
                
                
                
                gtk_window_set_accept_focus(GTK_WINDOW(mShell), FALSE);
#ifdef MOZ_X11
                
                
                gtk_widget_realize(mShell);
                gdk_window_add_filter(mShell->window,
                                      popup_take_focus_filter, NULL); 
#endif
            }

            GdkWindowTypeHint gtkTypeHint;
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
            gtk_window_set_type_hint(GTK_WINDOW(mShell), gtkTypeHint);

            if (topLevelParent) {
                gtk_window_set_transient_for(GTK_WINDOW(mShell),
                                            topLevelParent);
                mTransientParent = topLevelParent;

                if (topLevelParent->group) {
                    gtk_window_group_add_window(topLevelParent->group,
                                            GTK_WINDOW(mShell));
                    mWindowGroup = topLevelParent->group;
                    g_object_ref(mWindowGroup);
                }
            }
        }
        else { 
            mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            SetDefaultIcon();
            gtk_window_set_wmclass(GTK_WINDOW(mShell), "Toplevel", cBrand.get());

            
            mWindowGroup = gtk_window_group_new();

            
            LOG(("adding window %p to new group %p\n",
                 (void *)mShell, (void *)mWindowGroup));
            gtk_window_group_add_window(mWindowGroup, GTK_WINDOW(mShell));
        }

        
        GtkWidget *container = moz_container_new();
        mContainer = MOZ_CONTAINER(container);
        gtk_container_add(GTK_CONTAINER(mShell), container);
        gtk_widget_realize(container);

        
        GTK_PRIVATE_SET_FLAG(container, GTK_HAS_SHAPE_MASK);

        
        gtk_window_set_focus(GTK_WINDOW(mShell), container);

        
        mGdkWindow = container->window;

        if (mWindowType == eWindowType_popup) {
            
            

            mCursor = eCursor_wait; 
                                    
                                    
                                    
            SetCursor(eCursor_standard);

            if (aInitData->mNoAutoHide) {
                gint wmd = ConvertBorderStyles(mBorderStyle);
                if (wmd != -1)
                  gdk_window_set_decorations(mShell->window, (GdkWMDecoration) wmd);
            }
        }
    }
        break;
    case eWindowType_plugin:
    case eWindowType_child: {
        if (parentMozContainer) {
            mGdkWindow = CreateGdkWindow(parentGdkWindow, parentMozContainer);
            nsWindow *parentnsWindow =
                get_window_for_gdk_window(parentGdkWindow);
            if (parentnsWindow)
                mHasMappedToplevel = parentnsWindow->mHasMappedToplevel;
        }
        else if (parentGtkContainer) {
            GtkWidget *container = moz_container_new();
            mContainer = MOZ_CONTAINER(container);
            gtk_container_add(parentGtkContainer, container);
            gtk_widget_realize(container);

            
            GTK_PRIVATE_SET_FLAG(container, GTK_HAS_SHAPE_MASK);

            mGdkWindow = container->window;
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
    
    
    
    
    
#ifdef MOZ_X11
    if (mContainer)
        gtk_widget_set_double_buffered (GTK_WIDGET(mContainer),FALSE);
#endif

    
    g_object_set_data(G_OBJECT(mGdkWindow), "nsWindow", this);

    if (mContainer)
        g_object_set_data(G_OBJECT(mContainer), "nsWindow", this);

    if (mShell)
        g_object_set_data(G_OBJECT(mShell), "nsWindow", this);

    
    if (mShell) {
        g_signal_connect(mShell, "configure_event",
                         G_CALLBACK(configure_event_cb), NULL);
        g_signal_connect(mShell, "delete_event",
                         G_CALLBACK(delete_event_cb), NULL);
        g_signal_connect(mShell, "window_state_event",
                         G_CALLBACK(window_state_event_cb), NULL);

        GtkSettings* default_settings = gtk_settings_get_default();
        g_signal_connect_after(default_settings,
                               "notify::gtk-theme-name",
                               G_CALLBACK(theme_changed_cb), this);
        g_signal_connect_after(default_settings,
                               "notify::gtk-font-name",
                               G_CALLBACK(theme_changed_cb), this);

#ifdef MOZ_PLATFORM_MAEMO
        if (mWindowType == eWindowType_toplevel) {
            GdkWindow *gdkwin = mShell->window;

            
            gulong portrait_set = 1;
            GdkAtom support = gdk_atom_intern("_HILDON_PORTRAIT_MODE_SUPPORT", FALSE);
            gdk_property_change(gdkwin, support, gdk_x11_xatom_to_atom(XA_CARDINAL),
                                32, GDK_PROP_MODE_REPLACE,
                                (const guchar *) &portrait_set, 1);

            
            gulong volume_set = 1;
            GdkAtom keys = gdk_atom_intern("_HILDON_ZOOM_KEY_ATOM", FALSE);
            gdk_property_change(gdkwin, keys, gdk_x11_xatom_to_atom(XA_INTEGER),
                                32, GDK_PROP_MODE_REPLACE, (const guchar *) &volume_set, 1);
        }
#endif
    }

    if (mContainer) {
        g_signal_connect(mContainer, "unrealize",
                         G_CALLBACK(container_unrealize_cb), NULL);
        g_signal_connect_after(mContainer, "size_allocate",
                               G_CALLBACK(size_allocate_cb), NULL);
        g_signal_connect(mContainer, "expose_event",
                         G_CALLBACK(expose_event_cb), NULL);
        g_signal_connect(mContainer, "enter_notify_event",
                         G_CALLBACK(enter_notify_event_cb), NULL);
        g_signal_connect(mContainer, "leave_notify_event",
                         G_CALLBACK(leave_notify_event_cb), NULL);
        g_signal_connect(mContainer, "motion_notify_event",
                         G_CALLBACK(motion_notify_event_cb), NULL);
        g_signal_connect(mContainer, "button_press_event",
                         G_CALLBACK(button_press_event_cb), NULL);
        g_signal_connect(mContainer, "button_release_event",
                         G_CALLBACK(button_release_event_cb), NULL);
        g_signal_connect(mContainer, "focus_in_event",
                         G_CALLBACK(focus_in_event_cb), NULL);
        g_signal_connect(mContainer, "focus_out_event",
                         G_CALLBACK(focus_out_event_cb), NULL);
        g_signal_connect(mContainer, "key_press_event",
                         G_CALLBACK(key_press_event_cb), NULL);
        g_signal_connect(mContainer, "key_release_event",
                         G_CALLBACK(key_release_event_cb), NULL);
        g_signal_connect(mContainer, "scroll_event",
                         G_CALLBACK(scroll_event_cb), NULL);
        g_signal_connect(mContainer, "visibility_notify_event",
                         G_CALLBACK(visibility_notify_event_cb), NULL);
        g_signal_connect(mContainer, "hierarchy_changed",
                         G_CALLBACK(hierarchy_changed_cb), NULL);
        
        hierarchy_changed_cb(GTK_WIDGET(mContainer), NULL);

        gtk_drag_dest_set((GtkWidget *)mContainer,
                          (GtkDestDefaults)0,
                          NULL,
                          0,
                          (GdkDragAction)0);

        g_signal_connect(mContainer, "drag_motion",
                         G_CALLBACK(drag_motion_event_cb), NULL);
        g_signal_connect(mContainer, "drag_leave",
                         G_CALLBACK(drag_leave_event_cb), NULL);
        g_signal_connect(mContainer, "drag_drop",
                         G_CALLBACK(drag_drop_event_cb), NULL);
        g_signal_connect(mContainer, "drag_data_received",
                         G_CALLBACK(drag_data_received_event_cb), NULL);

        
        
        if (mWindowType != eWindowType_popup) {
            mIMModule = new nsGtkIMModule(this);
        }
    } else if (!mIMModule) {
        nsWindow *container = GetContainerWindow();
        if (container) {
            mIMModule = container->mIMModule;
        }
    }

    LOG(("nsWindow [%p]\n", (void *)this));
    if (mShell) {
        LOG(("\tmShell %p %p %lx\n", (void *)mShell, (void *)mShell->window,
             GDK_WINDOW_XWINDOW(mShell->window)));
    }

    if (mContainer) {
        LOG(("\tmContainer %p %p %lx\n", (void *)mContainer,
             (void *)GTK_WIDGET(mContainer)->window,
             GDK_WINDOW_XWINDOW(GTK_WIDGET(mContainer)->window)));
    }
    else if (mGdkWindow) {
        LOG(("\tmGdkWindow %p %lx\n", (void *)mGdkWindow,
             GDK_WINDOW_XWINDOW(mGdkWindow)));
    }

    
    if (!mIsTopLevel)
        Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_FALSE);

#ifdef ACCESSIBILITY
    nsresult rv;
    if (!sAccessibilityChecked) {
        sAccessibilityChecked = PR_TRUE;

        
        const char *envValue = PR_GetEnv(sAccEnv);
        if (envValue) {
            sAccessibilityEnabled = atoi(envValue) != 0;
            LOG(("Accessibility Env %s=%s\n", sAccEnv, envValue));
        }
        
        else {
            nsCOMPtr<nsIPrefBranch> sysPrefService =
                do_GetService(sSysPrefService, &rv);
            if (NS_SUCCEEDED(rv) && sysPrefService) {

                
                
                sysPrefService->GetBoolPref(sAccessibilityKey,
                                            &sAccessibilityEnabled);
            }

        }
    }
    if (sAccessibilityEnabled) {
        LOG(("nsWindow:: Create Toplevel Accessibility\n"));
        CreateRootAccessible();
    }
#endif

#ifdef MOZ_DFB
    if (!mDFB) {
         DirectFBCreate( &mDFB );

         D_ASSUME( mDFB != NULL );

         if (mDFB)
              mDFB->GetDisplayLayer( mDFB, DLID_PRIMARY, &mDFBLayer );

         D_ASSUME( mDFBLayer != NULL );

         if (mDFBLayer)
              mDFBLayer->GetCursorPosition( mDFBLayer, &mDFBCursorX, &mDFBCursorY );
    }
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString &xulWinType)
{
  if (!mShell)
    return NS_ERROR_FAILURE;

#ifdef MOZ_X11
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

  gdk_window_set_role(GTK_WIDGET(mShell)->window, role);
  
  
  XSetClassHint(GDK_DISPLAY(),
                GDK_WINDOW_XWINDOW(GTK_WIDGET(mShell)->window),
                class_hint);
  nsMemory::Free(class_hint->res_class);
  nsMemory::Free(class_hint->res_name);
  XFree(class_hint);
#else 

  char *res_name;

  res_name = ToNewCString(xulWinType);
  if (!res_name)
    return NS_ERROR_OUT_OF_MEMORY;

  printf("WARN: res_name = '%s'\n", res_name);


  const char *role = NULL;

  
  
  
  
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

  gdk_window_set_role(GTK_WIDGET(mShell)->window, role);

  nsMemory::Free(res_name);

#endif 
  return NS_OK;
}

void
nsWindow::NativeResize(PRInt32 aWidth, PRInt32 aHeight, PRBool  aRepaint)
{
    LOG(("nsWindow::NativeResize [%p] %d %d\n", (void *)this,
         aWidth, aHeight));

    ResizeTransparencyBitmap(aWidth, aHeight);

    
    mNeedsResize = PR_FALSE;

    if (mIsTopLevel) {
        gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
    }
    else if (mContainer) {
        GtkWidget *widget = GTK_WIDGET(mContainer);
        GtkAllocation allocation;
        allocation.x = widget->allocation.x;
        allocation.y = widget->allocation.y;
        allocation.width = aWidth;
        allocation.height = aHeight;
        gtk_widget_size_allocate(widget, &allocation);
    }
    else if (mGdkWindow) {
        gdk_window_resize(mGdkWindow, aWidth, aHeight);
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

    if (mIsTopLevel) {
        gtk_window_move(GTK_WINDOW(mShell), aX, aY);
        gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
    }
    else if (mContainer) {
        GtkAllocation allocation;
        allocation.x = aX;
        allocation.y = aY;
        allocation.width = aWidth;
        allocation.height = aHeight;
        gtk_widget_size_allocate(GTK_WIDGET(mContainer), &allocation);
    }
    else if (mGdkWindow) {
        gdk_window_move_resize(mGdkWindow, aX, aY, aWidth, aHeight);
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

        if (mIsTopLevel) {
            
            if (mWindowType != eWindowType_invisible) {
                SetUserTimeAndStartupIDForActivatedWindow(mShell);
            }

            gtk_widget_show(GTK_WIDGET(mContainer));
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
            gtk_widget_hide(GTK_WIDGET(mContainer));
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
nsWindow::SetHasMappedToplevel(PRBool aState)
{
    
    
    
    PRBool oldState = mHasMappedToplevel;
    mHasMappedToplevel = aState;

    
    
    
    
    
    
    
    if (!mIsShown || !mGdkWindow)
        return;

    if (aState && !oldState && !mIsFullyObscured) {
        
        
        
        gdk_window_invalidate_rect(mGdkWindow, NULL, FALSE);

        
        
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

nsIntSize
nsWindow::GetSafeWindowSize(nsIntSize aSize)
{
    nsIntSize result = aSize;
    const PRInt32 kInt16Max = 32767;
    if (result.width > kInt16Max) {
        NS_WARNING("Clamping huge window width");
        result.width = kInt16Max;
    }
    if (result.height > kInt16Max) {
        NS_WARNING("Clamping huge window height");
        result.height = kInt16Max;
    }
    return result;
}

void
nsWindow::EnsureGrabs(void)
{
    if (mRetryPointerGrab)
        GrabPointer();
}

void
nsWindow::SetTransparencyMode(nsTransparencyMode aMode)
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
        if (!topWidget)
            return;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return;

        topWindow->SetTransparencyMode(aMode);
        return;
    }
    PRBool isTransparent = aMode == eTransparencyTransparent;

    if (mIsTransparent == isTransparent)
        return;

    if (!isTransparent) {
        if (mTransparencyBitmap) {
            delete[] mTransparencyBitmap;
            mTransparencyBitmap = nsnull;
            mTransparencyBitmapWidth = 0;
            mTransparencyBitmapHeight = 0;
            gtk_widget_reset_shapes(mShell);
        }
    } 
    

    mIsTransparent = isTransparent;
}

nsTransparencyMode
nsWindow::GetTransparencyMode()
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
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
    for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
        const Configuration& configuration = aConfigurations[i];
        nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
        NS_ASSERTION(w->GetParent() == this,
                     "Configured widget is not a child");
        w->SetWindowClipRegion(configuration.mClipRegion, PR_TRUE);
        if (w->mBounds.Size() != configuration.mBounds.Size()) {
            w->Resize(configuration.mBounds.x, configuration.mBounds.y,
                      configuration.mBounds.width, configuration.mBounds.height,
                      PR_TRUE);
        } else if (w->mBounds.TopLeft() != configuration.mBounds.TopLeft()) {
            w->Move(configuration.mBounds.x, configuration.mBounds.y);
        } 
        w->SetWindowClipRegion(configuration.mClipRegion, PR_FALSE);
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
    for (PRUint32 i = 0; i < aRects.Length (); ++i) {
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

void
nsWindow::SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                              PRBool aIntersectWithExisting)
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
            return;
        }

        if (!pixman_region32_equal(&intersectRegion, &newRegion)) {
            GetIntRects(intersectRegion, &intersectRects);
            newRects = &intersectRects;
        }
    }

    if (!StoreWindowClipRegion(*newRects))
        return;

    if (!mGdkWindow)
        return;

    GdkRegion *region = gdk_region_new(); 
    for (PRUint32 i = 0; i < newRects->Length(); ++i) {
        const nsIntRect& r = newRects->ElementAt(i);
        GdkRectangle rect = { r.x, r.y, r.width, r.height };
        gdk_region_union_with_rect(region, &rect);
    }

    gdk_window_shape_combine_region(mGdkWindow, region, 0, 0);
    gdk_region_destroy(region);

    return;
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
    gchar* newBits = new gchar[newSize];
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
    gchar* fromPtr = mTransparencyBitmap;
    gchar* toPtr = newBits;
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
ChangedMaskBits(gchar* aMaskBits, PRInt32 aMaskWidth, PRInt32 aMaskHeight,
        const nsIntRect& aRect, PRUint8* aAlphas, PRInt32 aStride)
{
    PRInt32 x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
    PRInt32 maskBytesPerRow = (aMaskWidth + 7)/8;
    for (y = aRect.y; y < yMax; y++) {
        gchar* maskBytes = aMaskBits + y*maskBytesPerRow;
        PRUint8* alphas = aAlphas;
        for (x = aRect.x; x < xMax; x++) {
            PRBool newBit = *alphas > 0;
            alphas++;

            gchar maskByte = maskBytes[x >> 3];
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
void UpdateMaskBits(gchar* aMaskBits, PRInt32 aMaskWidth, PRInt32 aMaskHeight,
        const nsIntRect& aRect, PRUint8* aAlphas, PRInt32 aStride)
{
    PRInt32 x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
    PRInt32 maskBytesPerRow = (aMaskWidth + 7)/8;
    for (y = aRect.y; y < yMax; y++) {
        gchar* maskBytes = aMaskBits + y*maskBytesPerRow;
        PRUint8* alphas = aAlphas;
        for (x = aRect.x; x < xMax; x++) {
            PRBool newBit = *alphas > 0;
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
    
    
    
    Display* xDisplay = GDK_WINDOW_XDISPLAY(mShell->window);
    Window xDrawable = GDK_WINDOW_XID(mShell->window);
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
    gtk_widget_reset_shapes(mShell);
    GdkBitmap* maskBitmap = gdk_bitmap_create_from_data(mShell->window,
            mTransparencyBitmap,
            mTransparencyBitmapWidth, mTransparencyBitmapHeight);
    if (!maskBitmap)
        return;

    gtk_widget_shape_combine_mask(mShell, maskBitmap, 0, 0);
    g_object_unref(maskBitmap);
#endif
}

nsresult
nsWindow::UpdateTranslucentWindowAlphaInternal(const nsIntRect& aRect,
                                               PRUint8* aAlphas, PRInt32 aStride)
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
        if (!topWidget)
            return NS_ERROR_FAILURE;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return NS_ERROR_FAILURE;

        return topWindow->UpdateTranslucentWindowAlphaInternal(aRect, aAlphas, aStride);
    }

    NS_ASSERTION(mIsTransparent, "Window is not transparent");

    if (mTransparencyBitmap == nsnull) {
        PRInt32 size = ((mBounds.width+7)/8)*mBounds.height;
        mTransparencyBitmap = new gchar[size];
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

    
    
    
    if (!mHasMappedToplevel || mIsFullyObscured) {
        LOG(("GrabPointer: window not visible\n"));
        mRetryPointerGrab = PR_TRUE;
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
#ifdef HAVE_GTK_MOTION_HINTS
                                             GDK_POINTER_MOTION_HINT_MASK |
#endif
                                             GDK_POINTER_MOTION_MASK),
                              (GdkWindow *)NULL, NULL, GDK_CURRENT_TIME);

    if (retval != GDK_GRAB_SUCCESS) {
        LOG(("GrabPointer: pointer grab failed\n"));
        mRetryPointerGrab = PR_TRUE;
    }
}

void
nsWindow::ReleaseGrabs(void)
{
    LOG(("ReleaseGrabs\n"));

    mRetryPointerGrab = PR_FALSE;
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
}

void
nsWindow::GetToplevelWidget(GtkWidget **aWidget)
{
    *aWidget = nsnull;

    if (mShell) {
        *aWidget = mShell;
        return;
    }

    GtkWidget *widget = GetMozContainerWidget();
    if (!widget)
        return;

    *aWidget = gtk_widget_get_toplevel(widget);
}

GtkWidget *
nsWindow::GetMozContainerWidget()
{
    if (!mGdkWindow)
        return NULL;

    GtkWidget *owningWidget =
        get_gtk_widget_for_gdk_window(mGdkWindow);
    return owningWidget;
}

nsWindow *
nsWindow::GetContainerWindow()
{
    GtkWidget *owningWidget = GetMozContainerWidget();
    if (!owningWidget)
        return nsnull;

    nsWindow *window = get_window_for_gtk_widget(owningWidget);
    NS_ASSERTION(window, "No nsWindow for container widget");
    return window;
}

void
nsWindow::SetUrgencyHint(GtkWidget *top_window, PRBool state)
{
    if (!top_window)
        return;

    
    PRLibrary* lib;
    _gdk_window_set_urgency_hint_fn _gdk_window_set_urgency_hint = nsnull;
    _gdk_window_set_urgency_hint = (_gdk_window_set_urgency_hint_fn)
           PR_FindFunctionSymbolAndLibrary("gdk_window_set_urgency_hint", &lib);

    if (_gdk_window_set_urgency_hint) {
        _gdk_window_set_urgency_hint(top_window->window, state);
        PR_UnloadLibrary(lib);
    }
    else if (state) {
        gdk_window_show_unraised(top_window->window);
    }
}

void *
nsWindow::SetupPluginPort(void)
{
    if (!mGdkWindow)
        return nsnull;

    if (GDK_WINDOW_OBJECT(mGdkWindow)->destroyed == TRUE)
        return nsnull;

    
    
    
#ifdef MOZ_X11
    XWindowAttributes xattrs;
    XGetWindowAttributes(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(mGdkWindow),
                         &xattrs);
    XSelectInput (GDK_DISPLAY (),
                  GDK_WINDOW_XWINDOW(mGdkWindow),
                  xattrs.your_event_mask |
                  SubstructureNotifyMask);

    gdk_window_add_filter(mGdkWindow, plugin_window_filter_func, this);

    XSync(GDK_DISPLAY(), False);
#endif 

    return (void *)GDK_WINDOW_XWINDOW(mGdkWindow);
}

nsresult
nsWindow::SetWindowIconList(const nsTArray<nsCString> &aIconList)
{
    GList *list = NULL;

    for (PRUint32 i = 0; i < aIconList.Length(); ++i) {
        const char *path = aIconList[i].get();
        LOG(("window [%p] Loading icon from %s\n", (void *)this, path));

        GdkPixbuf *icon = gdk_pixbuf_new_from_file(path, NULL);
        if (!icon)
            continue;

        list = g_list_append(list, icon);
    }

    if (!list)
        return NS_ERROR_FAILURE;

    gtk_window_set_icon_list(GTK_WINDOW(mShell), list);

    g_list_foreach(list, (GFunc) g_object_unref, NULL);
    g_list_free(list);

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

    XGetInputFocus(GDK_WINDOW_XDISPLAY(mGdkWindow),
                   &curFocusWindow,
                   &focusState);

    LOGFOCUS(("\t curFocusWindow=%p\n", curFocusWindow));

    GdkWindow* toplevel = gdk_window_get_toplevel(mGdkWindow);
    GdkWindow *gdkfocuswin = gdk_window_lookup(curFocusWindow);

    
    
    
    if (gdkfocuswin != toplevel) {
        return;
    }

    
    mOldFocusWindow = curFocusWindow;
    XRaiseWindow(GDK_WINDOW_XDISPLAY(mGdkWindow),
                 GDK_WINDOW_XWINDOW(mGdkWindow));
    gdk_error_trap_push();
    XSetInputFocus(GDK_WINDOW_XDISPLAY(mGdkWindow),
                   GDK_WINDOW_XWINDOW(mGdkWindow),
                   RevertToNone,
                   CurrentTime);
    gdk_flush();
    gdk_error_trap_pop();
    gPluginFocusWindow = this;
    gdk_window_add_filter(NULL, plugin_client_message_filter, this);

    LOGFOCUS(("nsWindow::SetNonXEmbedPluginFocus oldfocus=%p new=%p\n",
              mOldFocusWindow, GDK_WINDOW_XWINDOW(mGdkWindow)));
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
        curFocusWindow == GDK_WINDOW_XWINDOW(mGdkWindow)) {

        gdk_error_trap_push();
        XRaiseWindow(GDK_WINDOW_XDISPLAY(mGdkWindow),
                     mOldFocusWindow);
        XSetInputFocus(GDK_WINDOW_XDISPLAY(mGdkWindow),
                       mOldFocusWindow,
                       RevertToParent,
                       CurrentTime);
        gdk_flush();
        gdk_error_trap_pop();
    }
    gPluginFocusWindow = NULL;
    mOldFocusWindow = 0;
    gdk_window_remove_filter(NULL, plugin_client_message_filter, this);

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
nsWindow::MakeFullScreen(PRBool aFullScreen)
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
nsWindow::HideWindowChrome(PRBool aShouldHide)
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
        if (!topWidget)
            return NS_ERROR_FAILURE;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return NS_ERROR_FAILURE;

        return topWindow->HideWindowChrome(aShouldHide);
    }

    
    
    
    PRBool wasVisible = PR_FALSE;
    if (gdk_window_is_visible(mShell->window)) {
        gdk_window_hide(mShell->window);
        wasVisible = PR_TRUE;
    }

    gint wmd;
    if (aShouldHide)
        wmd = 0;
    else
        wmd = ConvertBorderStyles(mBorderStyle);

    if (wmd != -1)
      gdk_window_set_decorations(mShell->window, (GdkWMDecoration) wmd);

    if (wasVisible)
        gdk_window_show(mShell->window);

    
    
    
    
    
#ifdef MOZ_X11
    XSync(GDK_DISPLAY(), False);
#else
    gdk_flush ();
#endif 

    return NS_OK;
}

PRBool
check_for_rollup(GdkWindow *aWindow, gdouble aMouseX, gdouble aMouseY,
                 PRBool aIsWheel, PRBool aAlwaysRollup)
{
    PRBool retVal = PR_FALSE;
    nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);

    if (rollupWidget && gRollupListener) {
        GdkWindow *currentPopup =
            (GdkWindow *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);
        if (aAlwaysRollup || !is_mouse_in_window(currentPopup, aMouseX, aMouseY)) {
            PRBool rollup = PR_TRUE;
            if (aIsWheel) {
                gRollupListener->ShouldRollupOnMouseWheelEvent(&rollup);
                retVal = PR_TRUE;
            }
            
            
            
            PRUint32 popupsToRollup = PR_UINT32_MAX;
            if (gMenuRollup && !aAlwaysRollup) {
                nsAutoTArray<nsIWidget*, 5> widgetChain;
                PRUint32 sameTypeCount = gMenuRollup->GetSubmenuWidgetChain(&widgetChain);
                for (PRUint32 i=0; i<widgetChain.Length(); ++i) {
                    nsIWidget* widget = widgetChain[i];
                    GdkWindow* currWindow =
                        (GdkWindow*) widget->GetNativeData(NS_NATIVE_WINDOW);
                    if (is_mouse_in_window(currWindow, aMouseX, aMouseY)) {
                      
                      
                      
                      
                      
                      if (i < sameTypeCount) {
                        rollup = PR_FALSE;
                      }
                      else {
                        popupsToRollup = sameTypeCount;
                      }
                      break;
                    }
                } 
            } 

            
            if (rollup) {
                gRollupListener->Rollup(popupsToRollup, nsnull);
                if (popupsToRollup == PR_UINT32_MAX) {
                    retVal = PR_TRUE;
                }
            }
        }
    } else {
        gRollupWindow = nsnull;
        gRollupListener = nsnull;
        NS_IF_RELEASE(gMenuRollup);
    }

    return retVal;
}


PRBool
nsWindow::DragInProgress(void)
{
    
    
    
    return (sLastDragMotionWindow || sIsDraggingOutOf);
}


PRBool
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

    gdk_drawable_get_size(aWindow, &w, &h);

    if (aMouseX > x && aMouseX < x + w &&
        aMouseY > y && aMouseY < y + h)
        return PR_TRUE;

    return PR_FALSE;
}


nsWindow *
get_window_for_gtk_widget(GtkWidget *widget)
{
    gpointer user_data = g_object_get_data(G_OBJECT(widget), "nsWindow");

    return static_cast<nsWindow *>(user_data);
}


nsWindow *
get_window_for_gdk_window(GdkWindow *window)
{
    gpointer user_data = g_object_get_data(G_OBJECT(window), "nsWindow");

    return static_cast<nsWindow *>(user_data);
}


GtkWidget *
get_gtk_widget_for_gdk_window(GdkWindow *window)
{
    gpointer user_data = NULL;
    gdk_window_get_user_data(window, &user_data);

    return GTK_WIDGET(user_data);
}


GdkCursor *
get_gtk_cursor(nsCursor aCursor)
{
    GdkPixmap *cursor;
    GdkPixmap *mask;
    GdkColor fg, bg;
    GdkCursor *gdkcursor = nsnull;
    PRUint8 newType = 0xff;

    if ((gdkcursor = gCursorCache[aCursor])) {
        return gdkcursor;
    }

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
        newType = MOZ_CURSOR_COPY;
        break;
    case eCursor_alias:
        newType = MOZ_CURSOR_ALIAS;
        break;
    case eCursor_context_menu:
        newType = MOZ_CURSOR_CONTEXT_MENU;
        break;
    case eCursor_cell:
        gdkcursor = gdk_cursor_new(GDK_PLUS);
        break;
    case eCursor_grab:
        newType = MOZ_CURSOR_HAND_GRAB;
        break;
    case eCursor_grabbing:
        newType = MOZ_CURSOR_HAND_GRABBING;
        break;
    case eCursor_spinning:
        newType = MOZ_CURSOR_SPINNING;
        break;
    case eCursor_zoom_in:
        newType = MOZ_CURSOR_ZOOM_IN;
        break;
    case eCursor_zoom_out:
        newType = MOZ_CURSOR_ZOOM_OUT;
        break;
    case eCursor_not_allowed:
    case eCursor_no_drop:
        newType = MOZ_CURSOR_NOT_ALLOWED;
        break;
    case eCursor_vertical_text:
        newType = MOZ_CURSOR_VERTICAL_TEXT;
        break;
    case eCursor_all_scroll:
        gdkcursor = gdk_cursor_new(GDK_FLEUR);
        break;
    case eCursor_nesw_resize:
        newType = MOZ_CURSOR_NESW_RESIZE;
        break;
    case eCursor_nwse_resize:
        newType = MOZ_CURSOR_NWSE_RESIZE;
        break;
    case eCursor_ns_resize:
    case eCursor_row_resize:
        gdkcursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
        break;
    case eCursor_ew_resize:
    case eCursor_col_resize:
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

    
    
    if (newType != 0xff) {
        gdk_color_parse("#000000", &fg);
        gdk_color_parse("#ffffff", &bg);

        cursor = gdk_bitmap_create_from_data(NULL,
                                             (char *)GtkCursors[newType].bits,
                                             32, 32);
        if (!cursor)
            return NULL;

        mask =
            gdk_bitmap_create_from_data(NULL,
                                        (char *)GtkCursors[newType].mask_bits,
                                        32, 32);
        if (!mask) {
            g_object_unref(cursor);
            return NULL;
        }

        gdkcursor = gdk_cursor_new_from_pixmap(cursor, mask, &fg, &bg,
                                               GtkCursors[newType].hot_x,
                                               GtkCursors[newType].hot_y);

        g_object_unref(mask);
        g_object_unref(cursor);
    }

    gCursorCache[aCursor] = gdkcursor;

    return gdkcursor;
}




gboolean
expose_event_cb(GtkWidget *widget, GdkEventExpose *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return FALSE;

    
    
    
    
    

    

    window->OnExposeEvent(widget, event);
    return FALSE;
}


gboolean
configure_event_cb(GtkWidget *widget,
                   GdkEventConfigure *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    return window->OnConfigureEvent(widget, event);
}


void
container_unrealize_cb (GtkWidget *widget)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return;

    window->OnContainerUnrealize(widget);
}


void
size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return;

    window->OnSizeAllocate(widget, allocation);
}


gboolean
delete_event_cb(GtkWidget *widget, GdkEventAny *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnDeleteEvent(widget, event);

    return TRUE;
}


gboolean
enter_notify_event_cb(GtkWidget *widget,
                      GdkEventCrossing *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

    window->OnEnterNotifyEvent(widget, event);

    return TRUE;
}


gboolean
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

    window->OnLeaveNotifyEvent(widget, event);

    return TRUE;
}

nsWindow*
GetFirstNSWindowForGDKWindow(GdkWindow *aGdkWindow)
{
    nsWindow* window;
    while (!(window = get_window_for_gdk_window(aGdkWindow))) {
        
        
        
        aGdkWindow = gdk_window_get_parent(aGdkWindow);
        if (!aGdkWindow) {
            window = nsnull;
            break;
        }
    }
    return window;
}


gboolean
motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event)
{
    UpdateLastInputEventTime();

    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnMotionNotifyEvent(widget, event);

#ifdef HAVE_GTK_MOTION_HINTS
    gdk_event_request_motions(event);
#endif
    return TRUE;
}


gboolean
button_press_event_cb(GtkWidget *widget, GdkEventButton *event)
{
    UpdateLastInputEventTime();

    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnButtonPressEvent(widget, event);

    return TRUE;
}


gboolean
button_release_event_cb(GtkWidget *widget, GdkEventButton *event)
{
    UpdateLastInputEventTime();

    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnButtonReleaseEvent(widget, event);

    return TRUE;
}


gboolean
focus_in_event_cb(GtkWidget *widget, GdkEventFocus *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnContainerFocusInEvent(widget, event);

    return FALSE;
}


gboolean
focus_out_event_cb(GtkWidget *widget, GdkEventFocus *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnContainerFocusOutEvent(widget, event);

    return FALSE;
}

#ifdef MOZ_X11


















GdkFilterReturn
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

    GdkWindow* parent_window = GTK_WIDGET(parent)->window;
    if (!parent_window)
        return GDK_FILTER_CONTINUE;

    
    gdk_window_show_unraised(parent_window);

    
    
    
    gdk_window_focus(parent_window, timestamp);
    return GDK_FILTER_REMOVE;
}


GdkFilterReturn
plugin_window_filter_func(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
    GdkWindow *plugin_window;
    XEvent    *xevent;

    nsRefPtr<nsWindow> nswindow = (nsWindow*)data;
    GdkFilterReturn return_val;

    xevent = (XEvent *)gdk_xevent;
    return_val = GDK_FILTER_CONTINUE;

    switch (xevent->type)
    {
        case CreateNotify:
        case ReparentNotify:
            if (xevent->type==CreateNotify) {
                plugin_window = gdk_window_lookup(xevent->xcreatewindow.window);
            }
            else {
                if (xevent->xreparent.event != xevent->xreparent.parent)
                    break;
                plugin_window = gdk_window_lookup (xevent->xreparent.window);
            }
            if (plugin_window) {
                GtkWidget *widget =
                    get_gtk_widget_for_gdk_window(plugin_window);

                if (GTK_IS_XTBIN(widget)) {
                    nswindow->SetPluginType(nsWindow::PluginType_NONXEMBED);
                    break;
                }
                else if(GTK_IS_SOCKET(widget)) {
                    nswindow->SetPluginType(nsWindow::PluginType_XEMBED);
                    break;
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


GdkFilterReturn
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


gboolean
key_press_event_cb(GtkWidget *widget, GdkEventKey *event)
{
    LOG(("key_press_event_cb\n"));

    UpdateLastInputEventTime();

    
    nsWindow *window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    nsRefPtr<nsWindow> focusWindow = gFocusWindow ? gFocusWindow : window;

#ifdef MOZ_X11
    
    
    
    
    
    
#define NS_GDKEVENT_MATCH_MASK 0x1FFF /* GDK_SHIFT_MASK .. GDK_BUTTON5_MASK */
    GdkDisplay* gdkDisplay = gtk_widget_get_display(widget);
    Display* dpy = GDK_DISPLAY_XDISPLAY(gdkDisplay);
    while (XPending(dpy)) {
        XEvent next_event;
        XPeekEvent(dpy, &next_event);
        GdkWindow* nextGdkWindow =
            gdk_window_lookup_for_display(gdkDisplay, next_event.xany.window);
        if (nextGdkWindow != event->window ||
            next_event.type != KeyPress ||
            next_event.xkey.keycode != event->hardware_keycode ||
            next_event.xkey.state != (event->state & NS_GDKEVENT_MATCH_MASK)) {
            break;
        }
        XNextEvent(dpy, &next_event);
        event->time = next_event.xkey.time;
    }
#endif

    return focusWindow->OnKeyPressEvent(widget, event);
}

gboolean
key_release_event_cb(GtkWidget *widget, GdkEventKey *event)
{
    LOG(("key_release_event_cb\n"));

    UpdateLastInputEventTime();

    
    nsWindow *window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    nsRefPtr<nsWindow> focusWindow = gFocusWindow ? gFocusWindow : window;

    return focusWindow->OnKeyReleaseEvent(widget, event);
}


gboolean
scroll_event_cb(GtkWidget *widget, GdkEventScroll *event)
{
    nsWindow *window = GetFirstNSWindowForGDKWindow(event->window);
    if (!window)
        return FALSE;

    window->OnScrollEvent(widget, event);

    return TRUE;
}


gboolean
visibility_notify_event_cb (GtkWidget *widget, GdkEventVisibility *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return FALSE;

    window->OnVisibilityNotifyEvent(widget, event);

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
        if (previous_toplevel->window) {
            old_window_state = gdk_window_get_state(previous_toplevel->window);
        }
    }

    if (GTK_IS_WINDOW(toplevel)) {
        g_signal_connect_swapped(toplevel, "window-state-event",
                                 G_CALLBACK(window_state_event_cb), widget);
        if (toplevel->window) {
            event.new_window_state = gdk_window_get_state(toplevel->window);
        }
    }

    event.changed_mask = static_cast<GdkWindowState>
        (old_window_state ^ event.new_window_state);

    if (event.changed_mask) {
        event.type = GDK_WINDOW_STATE;
        event.window = NULL;
        event.send_event = TRUE;
        window_state_event_cb(widget, &event);
    }
}


gboolean
window_state_event_cb (GtkWidget *widget, GdkEventWindowState *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    window->OnWindowStateEvent(widget, event);

    return FALSE;
}


void
theme_changed_cb (GtkSettings *settings, GParamSpec *pspec, nsWindow *data)
{
    nsRefPtr<nsWindow> window = data;
    window->ThemeChanged();
}




void
nsWindow::InitDragEvent(nsDragEvent &aEvent)
{
    
    gint x, y;
    GdkModifierType state = (GdkModifierType)0;
    gdk_window_get_pointer(NULL, &x, &y, &state);
    aEvent.isShift = (state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
    aEvent.isControl = (state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
    aEvent.isAlt = (state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
    aEvent.isMeta = PR_FALSE; 
}





void
nsWindow::UpdateDragStatus(GdkDragContext *aDragContext,
                           nsIDragService *aDragService)
{
    
    int action = nsIDragService::DRAGDROP_ACTION_NONE;

    
    if (aDragContext->actions & GDK_ACTION_DEFAULT)
        action = nsIDragService::DRAGDROP_ACTION_MOVE;

    
    if (aDragContext->actions & GDK_ACTION_MOVE)
        action = nsIDragService::DRAGDROP_ACTION_MOVE;

    
    else if (aDragContext->actions & GDK_ACTION_LINK)
        action = nsIDragService::DRAGDROP_ACTION_LINK;

    
    else if (aDragContext->actions & GDK_ACTION_COPY)
        action = nsIDragService::DRAGDROP_ACTION_COPY;

    
    nsCOMPtr<nsIDragSession> session;
    aDragService->GetCurrentSession(getter_AddRefs(session));

    if (session)
        session->SetDragAction(action);
}



gboolean
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

    return window->OnDragMotionEvent(aWidget,
                                     aDragContext,
                                     aX, aY, aTime, aData);
}

void
drag_leave_event_cb(GtkWidget *aWidget,
                    GdkDragContext *aDragContext,
                    guint aTime,
                    gpointer aData)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(aWidget);
    if (!window)
        return;

    window->OnDragLeaveEvent(aWidget, aDragContext, aTime, aData);
}



gboolean
drag_drop_event_cb(GtkWidget *aWidget,
                   GdkDragContext *aDragContext,
                   gint aX,
                   gint aY,
                   guint aTime,
                   gpointer *aData)
{
    nsRefPtr<nsWindow> window = get_window_for_gtk_widget(aWidget);
    if (!window)
        return FALSE;

    return window->OnDragDropEvent(aWidget,
                                   aDragContext,
                                   aX, aY, aTime, aData);
}


void
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


nsresult
initialize_prefs(void)
{
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (!prefs)
        return NS_OK;

    PRBool val = PR_TRUE;
    nsresult rv;

    rv = prefs->GetBoolPref("mozilla.widget.raise-on-setfocus", &val);
    if (NS_SUCCEEDED(rv))
        gRaiseWindows = val;

    rv = prefs->GetBoolPref("mozilla.widget.disable-native-theme", &val);
    if (NS_SUCCEEDED(rv))
        gDisableNativeTheme = val;

    return NS_OK;
}

void
nsWindow::FireDragLeaveTimer(void)
{
    LOGDRAG(("nsWindow::FireDragLeaveTimer(%p)\n", (void*)this));

    mDragLeaveTimer = nsnull;

    
    if (sLastDragMotionWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = sLastDragMotionWindow;
        
        sLastDragMotionWindow->OnDragLeave();
        sLastDragMotionWindow = 0;
    }
}


void
nsWindow::DragLeaveTimerCallback(nsITimer *aTimer, void *aClosure)
{
    nsRefPtr<nsWindow> window = static_cast<nsWindow *>(aClosure);
    window->FireDragLeaveTimer();
}


GdkWindow *
get_inner_gdk_window (GdkWindow *aWindow,
                      gint x, gint y,
                      gint *retx, gint *rety)
{
    gint cx, cy, cw, ch, cd;
    GList *children = gdk_window_peek_children(aWindow);
    for (GList *child = g_list_last(children);
         child;
         child = g_list_previous(child)) {
        GdkWindow *childWindow = (GdkWindow *) child->data;
        if (get_window_for_gdk_window(childWindow)) {
            gdk_window_get_geometry(childWindow, &cx, &cy, &cw, &ch, &cd);
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
                                GdkEventKey *aGdkEvent)
{
    aEvent.refPoint = nsIntPoint(0, 0);
    aEvent.isShift = PR_FALSE;
    aEvent.isControl = PR_FALSE;
    aEvent.isAlt = PR_FALSE;
    aEvent.isMeta = PR_FALSE;
    aEvent.time = aGdkEvent->time;
    aEvent.clickCount = 1;
}


int
is_parent_ungrab_enter(GdkEventCrossing *aEvent)
{
    return (GDK_CROSSING_UNGRAB == aEvent->mode) &&
        ((GDK_NOTIFY_ANCESTOR == aEvent->detail) ||
         (GDK_NOTIFY_VIRTUAL == aEvent->detail));

}


int
is_parent_grab_leave(GdkEventCrossing *aEvent)
{
    return (GDK_CROSSING_GRAB == aEvent->mode) &&
        ((GDK_NOTIFY_ANCESTOR == aEvent->detail) ||
            (GDK_NOTIFY_VIRTUAL == aEvent->detail));
}

static GdkModifierType
gdk_keyboard_get_modifiers()
{
    GdkModifierType m = (GdkModifierType) 0;

    gdk_window_get_pointer(NULL, NULL, NULL, &m);

    return m;
}

#ifdef MOZ_X11


static PRBool
gdk_keyboard_get_modmap_masks(Display*  aDisplay,
                              PRUint32* aCapsLockMask,
                              PRUint32* aNumLockMask,
                              PRUint32* aScrollLockMask)
{
    *aCapsLockMask = 0;
    *aNumLockMask = 0;
    *aScrollLockMask = 0;

    int min_keycode = 0;
    int max_keycode = 0;
    XDisplayKeycodes(aDisplay, &min_keycode, &max_keycode);

    int keysyms_per_keycode = 0;
    KeySym* xkeymap = XGetKeyboardMapping(aDisplay, min_keycode,
                                          max_keycode - min_keycode + 1,
                                          &keysyms_per_keycode);
    if (!xkeymap) {
        return PR_FALSE;
    }

    XModifierKeymap* xmodmap = XGetModifierMapping(aDisplay);
    if (!xmodmap) {
        XFree(xkeymap);
        return PR_FALSE;
    }

    





    const unsigned int map_size = 8 * xmodmap->max_keypermod;
    for (unsigned int i = 0; i < map_size; i++) {
        KeyCode keycode = xmodmap->modifiermap[i];
        if (!keycode || keycode < min_keycode || keycode > max_keycode)
            continue;

        const KeySym* syms = xkeymap + (keycode - min_keycode) * keysyms_per_keycode;
        const unsigned int mask = 1 << (i / xmodmap->max_keypermod);
        for (int j = 0; j < keysyms_per_keycode; j++) {
            switch (syms[j]) {
                case GDK_Caps_Lock:   *aCapsLockMask |= mask;   break;
                case GDK_Num_Lock:    *aNumLockMask |= mask;    break;
                case GDK_Scroll_Lock: *aScrollLockMask |= mask; break;
            }
        }
    }

    XFreeModifiermap(xmodmap);
    XFree(xkeymap);
    return PR_TRUE;
}
#endif 

#ifdef ACCESSIBILITY
void
nsWindow::CreateRootAccessible()
{
    if (mIsTopLevel && !mRootAccessible) {
        nsAccessible *acc = DispatchAccessibleEvent();

        if (acc) {
            mRootAccessible = acc;
        }
    }
}

nsAccessible*
nsWindow::DispatchAccessibleEvent()
{
    nsAccessibleEvent event(PR_TRUE, NS_GETACCESSIBLE, this);

    nsEventStatus status;
    DispatchEvent(&event, status);

    return event.mAccessible;
}

void
nsWindow::DispatchEventToRootAccessible(PRUint32 aEventType)
{
    if (!sAccessibilityEnabled) {
        return;
    }

    nsCOMPtr<nsIAccessibilityService> accService =
        do_GetService("@mozilla.org/accessibilityService;1");
    if (!accService) {
        return;
    }

    
    nsAccessible *acc = DispatchAccessibleEvent();
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

NS_IMETHODIMP
nsWindow::ResetInputState()
{
    return mIMModule ? mIMModule->ResetInputState(this) : NS_OK;
}

NS_IMETHODIMP
nsWindow::SetInputMode(const IMEContext& aContext)
{
    return mIMModule ? mIMModule->SetInputMode(this, &aContext) : NS_OK;
}

NS_IMETHODIMP
nsWindow::GetInputMode(IMEContext& aContext)
{
  if (!mIMModule) {
      aContext.mStatus = nsIWidget::IME_STATUS_DISABLED;
      return NS_OK;
  }
  return mIMModule->GetInputMode(&aContext);
}

NS_IMETHODIMP
nsWindow::CancelIMEComposition()
{
    return mIMModule ? mIMModule->CancelIMEComposition(this) : NS_OK;
}

NS_IMETHODIMP
nsWindow::OnIMEFocusChange(PRBool aFocus)
{
    if (mIMModule) {
      mIMModule->OnFocusChangeInGecko(aFocus);
    }
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState)
{
    NS_ENSURE_ARG_POINTER(aLEDState);

#ifdef MOZ_X11

    GdkModifierType modifiers = gdk_keyboard_get_modifiers();
    PRUint32 capsLockMask, numLockMask, scrollLockMask;
    PRBool foundMasks = gdk_keyboard_get_modmap_masks(
                          GDK_WINDOW_XDISPLAY(mGdkWindow),
                          &capsLockMask, &numLockMask, &scrollLockMask);
    if (!foundMasks)
        return NS_ERROR_NOT_IMPLEMENTED;

    PRUint32 mask = 0;
    switch (aKeyCode) {
        case NS_VK_CAPS_LOCK:   mask = capsLockMask;   break;
        case NS_VK_NUM_LOCK:    mask = numLockMask;    break;
        case NS_VK_SCROLL_LOCK: mask = scrollLockMask; break;
    }
    if (mask == 0)
        return NS_ERROR_NOT_IMPLEMENTED;

    *aLEDState = (modifiers & mask) != 0;
    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}

#ifdef MOZ_X11
 already_AddRefed<gfxASurface>
nsWindow::GetSurfaceForGdkDrawable(GdkDrawable* aDrawable,
                                   const nsIntSize& aSize)
{
    GdkVisual* visual = gdk_drawable_get_visual(aDrawable);
    Screen* xScreen =
        gdk_x11_screen_get_xscreen(gdk_drawable_get_screen(aDrawable));
    Display* xDisplay = DisplayOfScreen(xScreen);
    Drawable xDrawable = gdk_x11_drawable_get_xid(aDrawable);

    gfxASurface* result = nsnull;

    if (visual) {
        Visual* xVisual = gdk_x11_visual_get_xvisual(visual);

        result = new gfxXlibSurface(xDisplay, xDrawable, xVisual,
                                    gfxIntSize(aSize.width, aSize.height));
    } else {
        
        
        XRenderPictFormat *pf = NULL;
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

    NS_IF_ADDREF(result);
    return result;
}
#endif


gfxASurface*
nsWindow::GetThebesSurface()
{
    if (!mGdkWindow)
        return nsnull;

    GdkDrawable* d;
    gint x_offset, y_offset;
    gdk_window_get_internal_paint_info(mGdkWindow, &d, &x_offset, &y_offset);

#ifdef MOZ_X11
    gint width, height;
    gdk_drawable_get_size(d, &width, &height);
    
    width = PR_MIN(32767, width);
    height = PR_MIN(32767, height);
    gfxIntSize size(width, height);
    Visual* visual = GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(d));

#  ifdef MOZ_HAVE_SHMIMAGE
    PRBool usingShm = PR_FALSE;
    if (nsShmImage::UseShm()) {
        
        
        
        mThebesSurface =
            nsShmImage::EnsureShmImage(size,
                                       visual, gdk_drawable_get_depth(d),
                                       mShmImage);
        usingShm = mThebesSurface != nsnull;
    }
    if (!usingShm)
#  endif  

    mThebesSurface = new gfxXlibSurface
        (GDK_WINDOW_XDISPLAY(d),
         GDK_WINDOW_XWINDOW(d),
         visual,
         size);
#endif
#ifdef MOZ_DFB
    mThebesSurface = new gfxDirectFBSurface(gdk_directfb_surface_lookup(d));
#endif

    
    
    if (mThebesSurface && mThebesSurface->CairoStatus() != 0) {
        mThebesSurface = nsnull;
    } else {
        mThebesSurface->SetDeviceOffset(gfxPoint(-x_offset, -y_offset));
    }

    return mThebesSurface;
}


PRBool
nsWindow::GetDragInfo(nsMouseEvent* aMouseEvent,
                      GdkWindow** aWindow, gint* aButton,
                      gint* aRootX, gint* aRootY)
{
    if (aMouseEvent->button != nsMouseEvent::eLeftButton) {
        
        return PR_FALSE;
    }
    *aButton = 1;

    
    GdkWindow* gdk_window = mGdkWindow;
    if (!gdk_window) {
        return PR_FALSE;
    }
    NS_ABORT_IF_FALSE(GDK_IS_WINDOW(gdk_window), "must really be window");

    
    gdk_window = gdk_window_get_toplevel(gdk_window);
    NS_ABORT_IF_FALSE(gdk_window,
                      "gdk_window_get_toplevel should not return null");
    *aWindow = gdk_window;

    if (!aMouseEvent->widget) {
        return PR_FALSE;
    }

    
    
    
    
    
    nsIntPoint offset = aMouseEvent->widget->WidgetToScreenOffset();
    *aRootX = aMouseEvent->refPoint.x + offset.x;
    *aRootY = aMouseEvent->refPoint.y + offset.y;

    return PR_TRUE;
}

NS_IMETHODIMP
nsWindow::BeginMoveDrag(nsMouseEvent* aEvent)
{
    NS_ABORT_IF_FALSE(aEvent, "must have event");
    NS_ABORT_IF_FALSE(aEvent->eventStructType == NS_MOUSE_EVENT,
                      "event must have correct struct type");

    GdkWindow *gdk_window;
    gint button, screenX, screenY;
    if (!GetDragInfo(aEvent, &gdk_window, &button, &screenX, &screenY)) {
        return NS_ERROR_FAILURE;
    }

    
    gdk_window_begin_move_drag(gdk_window, button, screenX, screenY,
                               aEvent->time);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical)
{
    NS_ENSURE_ARG_POINTER(aEvent);

    if (aEvent->eventStructType != NS_MOUSE_EVENT) {
        
        return NS_ERROR_INVALID_ARG;
    }

    nsMouseEvent* mouse_event = static_cast<nsMouseEvent*>(aEvent);

    GdkWindow *gdk_window;
    gint button, screenX, screenY;
    if (!GetDragInfo(mouse_event, &gdk_window, &button, &screenX, &screenY)) {
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

void
nsWindow::ClearCachedResources()
{
    if (mLayerManager &&
        mLayerManager->GetBackendType() == LayerManager::LAYERS_BASIC) {
        static_cast<BasicLayerManager*> (mLayerManager.get())->
            ClearCachedResources();
    }

    GList* children = gdk_window_peek_children(mGdkWindow);
    for (GList* list = children; list; list = list->next) {
        nsWindow* window = get_window_for_gdk_window(GDK_WINDOW(list->data));
        if (window) {
            window->ClearCachedResources();
        }
    }
}
