







































#include "prlink.h"

#include "nsWindow.h"
#include "nsGTKToolkit.h"
#include "nsIDeviceContext.h"
#include "nsIRenderingContext.h"
#include "nsIRegion.h"
#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIDOMNode.h"

#include "nsWidgetsCID.h"
#include "nsIDragService.h"
#include "nsIDragSessionGTK.h"

#include "nsGtkKeyUtils.h"
#include "nsGtkCursors.h"

#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#ifdef MOZ_ENABLE_STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <startup-notification-1.0/libsn/sn.h>
#endif

#include "gtk2xtbin.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsGfxCIID.h"

#ifdef ACCESSIBILITY
#include "nsIAccessibleRole.h"
#include "nsPIAccessNode.h"
#include "nsPIAccessible.h"
#include "nsIAccessibleEvent.h"
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
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsGfxCIID.h"
#include "nsIImage.h"
#include "nsImageToPixbuf.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"

#include "gfxPlatformGtk.h"
#include "gfxXlibSurface.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

#ifdef MOZ_ENABLE_GLITZ
#include "gfxGlitzSurface.h"
#include "glitz-glx.h"
#endif


static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);


static PRBool     check_for_rollup(GdkWindow *aWindow,
                                   gdouble aMouseX, gdouble aMouseY,
                                   PRBool aIsWheel);
static PRBool     is_mouse_in_window(GdkWindow* aWindow,
                                     gdouble aMouseX, gdouble aMouseY);
static nsWindow  *get_window_for_gtk_widget(GtkWidget *widget);
static nsWindow  *get_window_for_gdk_window(GdkWindow *window);
static nsWindow  *get_owning_window_for_gdk_window(GdkWindow *window);
static GtkWidget *get_gtk_widget_for_gdk_window(GdkWindow *window);
static GdkCursor *get_gtk_cursor(nsCursor aCursor);

static GdkWindow *get_inner_gdk_window (GdkWindow *aWindow,
                                        gint x, gint y,
                                        gint *retx, gint *rety);

static inline PRBool is_context_menu_key(const nsKeyEvent& inKeyEvent);
static void   key_event_to_context_menu_event(const nsKeyEvent* inKeyEvent,
                                              nsMouseEvent* outCMEvent);

static int    is_parent_ungrab_enter(GdkEventCrossing *aEvent);
static int    is_parent_grab_leave(GdkEventCrossing *aEvent);


static gboolean expose_event_cb           (GtkWidget *widget,
                                           GdkEventExpose *event);
static gboolean configure_event_cb        (GtkWidget *widget,
                                           GdkEventConfigure *event);
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
static gboolean window_state_event_cb     (GtkWidget *widget,
                                           GdkEventWindowState *event);
static void     theme_changed_cb          (GtkSettings *settings,
                                           GParamSpec *pspec,
                                           nsWindow *data);
#ifdef __cplusplus
extern "C" {
#endif
static GdkFilterReturn plugin_window_filter_func (GdkXEvent *gdk_xevent,
                                                  GdkEvent *event,
                                                  gpointer data);
static GdkFilterReturn plugin_client_message_filter (GdkXEvent *xevent,
                                                     GdkEvent *event,
                                                     gpointer data);
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


static nsresult    initialize_prefs        (void);


nsWindow *nsWindow::mLastDragMotionWindow = NULL;
PRBool nsWindow::sIsDraggingOutOf = PR_FALSE;



guint32   nsWindow::mLastButtonPressTime = 0;


guint32   nsWindow::mLastButtonReleaseTime = 0;

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);


static nsWindow         *gFocusWindow          = NULL;
static PRBool            gGlobalsInitialized   = PR_FALSE;
static PRBool            gRaiseWindows         = PR_TRUE;
static nsWindow         *gPluginFocusWindow    = NULL;

nsCOMPtr  <nsIRollupListener> gRollupListener;
nsWeakPtr                     gRollupWindow;

#define NS_WINDOW_TITLE_MAX_LENGTH 4095

#ifdef USE_XIM

static nsWindow    *gIMEFocusWindow = NULL;
static GdkEventKey *gKeyEvent = NULL;
static PRBool       gKeyEventCommitted = PR_FALSE;
static PRBool       gKeyEventChanged = PR_FALSE;
static PRBool       gIMESuppressCommit = PR_FALSE;

static void IM_commit_cb              (GtkIMContext *aContext,
                                       const gchar *aString,
                                       nsWindow *aWindow);
static void IM_commit_cb_internal     (const gchar *aString,
                                       nsWindow *aWindow);
static void IM_preedit_changed_cb     (GtkIMContext *aContext,
                                       nsWindow *aWindow);
static void IM_set_text_range         (const PRInt32 aLen,
                                       const gchar *aPreeditString,
                                       const gint aCursorPos,
                                       const PangoAttrList *aFeedback,
                                       PRUint32 *aTextRangeListLengthResult,
                                       nsTextRangeArray *aTextRangeListResult);

static nsWindow *IM_get_owning_window(MozDrawingarea *aArea);

static GtkIMContext *IM_get_input_context(nsWindow *window);



#endif



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

nsWindow::nsWindow()
{
    mContainer           = nsnull;
    mDrawingarea         = nsnull;
    mShell               = nsnull;
    mWindowGroup         = nsnull;
    mContainerGotFocus   = PR_FALSE;
    mContainerLostFocus  = PR_FALSE;
    mContainerBlockFocus = PR_FALSE;
    mIsVisible           = PR_FALSE;
    mRetryPointerGrab    = PR_FALSE;
    mRetryKeyboardGrab   = PR_FALSE;
    mActivatePending     = PR_FALSE;
    mTransientParent     = nsnull;
    mWindowType          = eWindowType_child;
    mSizeState           = nsSizeMode_Normal;
    mOldFocusWindow      = 0;
    mPluginType          = PluginType_NONE;

    if (!gGlobalsInitialized) {
        gGlobalsInitialized = PR_TRUE;

        
        initialize_prefs();
    }

    memset(mKeyDownFlags, 0, sizeof(mKeyDownFlags));

    if (mLastDragMotionWindow == this)
        mLastDragMotionWindow = NULL;
    mDragMotionWidget = 0;
    mDragMotionContext = 0;
    mDragMotionX = 0;
    mDragMotionY = 0;
    mDragMotionTime = 0;
    mDragMotionTimerID = 0;

#ifdef USE_XIM
    mIMEData = nsnull;
#endif

#ifdef ACCESSIBILITY
    mRootAccessible  = nsnull;
#endif

    mIsTranslucent = PR_FALSE;
    mTransparencyBitmap = nsnull;

    mTransparencyBitmapWidth  = 0;
    mTransparencyBitmapHeight = 0;
}

nsWindow::~nsWindow()
{
    LOG(("nsWindow::~nsWindow() [%p]\n", (void *)this));
    if (mLastDragMotionWindow == this) {
        mLastDragMotionWindow = NULL;
    }

    delete[] mTransparencyBitmap;
    mTransparencyBitmap = nsnull;

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

#ifndef USE_XIM
NS_IMPL_ISUPPORTS_INHERITED1(nsWindow, nsCommonWidget,
                             nsISupportsWeakReference)
#else
NS_IMPL_ISUPPORTS_INHERITED2(nsWindow, nsCommonWidget,
                             nsISupportsWeakReference,
                             nsIKBStateControl)
#endif

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
    
    g_signal_handlers_disconnect_by_func(gtk_settings_get_default(),
                                         (gpointer)G_CALLBACK(theme_changed_cb),
                                         this);

    
    nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);
    if (NS_STATIC_CAST(nsIWidget *, this) == rollupWidget.get()) {
        if (gRollupListener)
            gRollupListener->Rollup();
        gRollupWindow = nsnull;
        gRollupListener = nsnull;
    }

    NativeShow(PR_FALSE);

    
    
    
    for (nsIWidget* kid = mFirstChild; kid; ) {
        nsIWidget* next = kid->GetNextSibling();
        kid->Destroy();
        kid = next;
    }

#ifdef USE_XIM
    IMEDestroyContext();
#endif

    
    if (gFocusWindow == this) {
        LOGFOCUS(("automatically losing focus...\n"));
        gFocusWindow = nsnull;
    }

    
    if (gPluginFocusWindow == this) {
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }

    
    
    
    
    mWindowGroup = nsnull;

    
    
    mThebesSurface = nsnull;

    if (mDragMotionTimerID) {
        gtk_timeout_remove(mDragMotionTimerID);
        mDragMotionTimerID = 0;
    }

    if (mShell) {
        gtk_widget_destroy(mShell);
        mShell = nsnull;
        mContainer = nsnull;
    }
    else if (mContainer) {
        gtk_widget_destroy(GTK_WIDGET(mContainer));
        mContainer = nsnull;
    }

    if (mDrawingarea) {
        g_object_unref(mDrawingarea);
        mDrawingarea = nsnull;
    }

    OnDestroy();

#ifdef ACCESSIBILITY
    if (mRootAccessible) {
        mRootAccessible = nsnull;
    }
#endif

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetParent(nsIWidget *aNewParent)
{
    NS_ENSURE_ARG_POINTER(aNewParent);

    GdkWindow* newParentWindow =
        NS_STATIC_CAST(GdkWindow*, aNewParent->GetNativeData(NS_NATIVE_WINDOW));
    NS_ASSERTION(newParentWindow, "Parent widget has a null native window handle");

    if (!mShell && mDrawingarea) {
        moz_drawingarea_reparent(mDrawingarea, newParentWindow);
    } else {
        NS_NOTREACHED("nsWindow::SetParent - reparenting a non-child window");
    }
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetModal(PRBool aModal)
{
    LOG(("nsWindow::SetModal [%p] %d\n", (void *)this, aModal));

    
    GtkWidget *grabWidget = nsnull;

    GetToplevelWidget(&grabWidget);

    if (!grabWidget)
        return NS_ERROR_FAILURE;

    if (aModal)
        gtk_grab_add(grabWidget);
    else
        gtk_grab_remove(grabWidget);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsVisible(PRBool & aState)
{
    aState = mIsVisible;
    if (mIsTopLevel && mShell) {
        aState = GTK_WIDGET_VISIBLE(mShell);
    }
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
nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
    LOG(("nsWindow::Move [%p] %d %d\n", (void *)this,
         aX, aY));

    mPlaced = PR_TRUE;

    
    
    
    if (aX == mBounds.x && aY == mBounds.y &&
        mWindowType != eWindowType_popup)
        return NS_OK;

    

    mBounds.x = aX;
    mBounds.y = aY;

    if (!mCreated)
        return NS_OK;

    if (mIsTopLevel) {
        if (mParent && mWindowType == eWindowType_popup) {
            nsRect oldrect, newrect;
            oldrect.x = aX;
            oldrect.y = aY;
            mParent->WidgetToScreen(oldrect, newrect);
            gtk_window_move(GTK_WINDOW(mShell), newrect.x, newrect.y);
        }
        else {
            gtk_window_move(GTK_WINDOW(mShell), aX, aY);
        }
    }
    else if (mDrawingarea) {
        moz_drawingarea_move(mDrawingarea, aX, aY);
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
        
        if (mDrawingarea)
            gdk_window_raise(mDrawingarea->clip_window);
    } else {
        
        for (nsWindow* w = this; w;
             w = NS_STATIC_CAST(nsWindow*, w->GetPrevSibling())) {
            if (w->mDrawingarea)
                gdk_window_lower(w->mDrawingarea->clip_window);
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

NS_IMETHODIMP
nsWindow::Enable(PRBool aState)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

typedef void (* SetUserTimeFunc)(GdkWindow* aWindow, guint32 aTimestamp);



static void
SetUserTimeAndStartupIDForActivatedWindow(GtkWidget* aWindow)
{
    nsCOMPtr<nsIToolkit> toolkit;
    NS_GetCurrentToolkit(getter_AddRefs(toolkit));
    if (!toolkit)
        return;

    nsGTKToolkit* GTKToolkit = NS_STATIC_CAST(nsGTKToolkit*,
        NS_STATIC_CAST(nsIToolkit*, toolkit));
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
    
    

    LOGFOCUS(("  SetFocus [%p]\n", (void *)this));

    if (!mDrawingarea)
        return NS_ERROR_FAILURE;

    GtkWidget *owningWidget =
        get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);
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

    if (!GTK_WIDGET_HAS_FOCUS(owningWidget)) {
        LOGFOCUS(("  grabbing focus for the toplevel [%p]\n", (void *)this));
        owningWindow->mContainerBlockFocus = PR_TRUE;
        
        
        if (gRaiseWindows && aRaise && toplevelWidget &&
            !GTK_WIDGET_HAS_FOCUS(toplevelWidget) &&
            owningWindow->mIsShown && GTK_IS_WINDOW(owningWindow->mShell))
          gtk_window_present(GTK_WINDOW(owningWindow->mShell));
        
        gtk_widget_grab_focus(owningWidget);
        owningWindow->mContainerBlockFocus = PR_FALSE;

        DispatchGotFocusEvent();

        
        if (owningWindow->mActivatePending) {
            owningWindow->mActivatePending = PR_FALSE;
            DispatchActivateEvent();
        }

        return NS_OK;
    }

    
    if (gFocusWindow == this) {
        LOGFOCUS(("  already have focus [%p]\n", (void *)this));
        return NS_OK;
    }

    
    
    if (gFocusWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gFocusWindow;
#ifdef USE_XIM
        
        
        
        if (IM_get_input_context(this) !=
            IM_get_input_context(gFocusWindow))
            gFocusWindow->IMELoseFocus();
#endif
        gFocusWindow->LoseFocus();
    }

    
    
    gFocusWindow = this;

#ifdef USE_XIM
    IMESetFocus();
#endif

    LOGFOCUS(("  widget now has focus - dispatching events [%p]\n",
              (void *)this));

    DispatchGotFocusEvent();

    
    if (owningWindow->mActivatePending) {
        owningWindow->mActivatePending = PR_FALSE;
        DispatchActivateEvent();
    }

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

nsIFontMetrics*
nsWindow::GetFont(void)
{
    return nsnull;
}

NS_IMETHODIMP
nsWindow::SetFont(const nsFont &aFont)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetCursor(nsCursor aCursor)
{
    
    
    if (!mContainer && mDrawingarea) {
        GtkWidget *widget =
            get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);
        nsWindow *window = get_window_for_gtk_widget(widget);
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
    
    
    if (!mContainer && mDrawingarea) {
        GtkWidget *widget =
            get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);
        nsWindow *window = get_window_for_gtk_widget(widget);
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

    
    nsCOMPtr<gfxIImageFrame> frame;
    aCursor->GetFrameAt(0, getter_AddRefs(frame));
    if (!frame)
        return NS_ERROR_NOT_AVAILABLE;

    nsCOMPtr<nsIImage> img(do_GetInterface(frame));
    if (!img)
        return NS_ERROR_NOT_AVAILABLE;

    GdkPixbuf* pixbuf = nsImageToPixbuf::ImageToPixbuf(img);
    if (!pixbuf)
        return NS_ERROR_NOT_AVAILABLE;

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    
    
    
    
    if (width > 128 || height > 128)
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    if (!gdk_pixbuf_get_has_alpha(pixbuf)) {
        GdkPixbuf* alphaBuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
        gdk_pixbuf_unref(pixbuf);
        if (!alphaBuf) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        pixbuf = alphaBuf;
    }

    GdkCursor* cursor;
    if (!_gdk_cursor_new_from_pixbuf || !_gdk_display_get_default) {
        
        GdkPixmap* mask = gdk_pixmap_new(NULL, width, height, 1);
        if (!mask)
            return NS_ERROR_OUT_OF_MEMORY;

        PRUint8* data = Data32BitTo1Bit(gdk_pixbuf_get_pixels(pixbuf),
                                        gdk_pixbuf_get_rowstride(pixbuf),
                                        width, height);
        if (!data) {
            g_object_unref(mask);
            return NS_ERROR_OUT_OF_MEMORY;
        }

        GdkPixmap* image = gdk_bitmap_create_from_data(NULL, (const gchar*)data, width,
                                                       height);
        delete[] data;
        if (!image) {
            g_object_unref(mask);
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
    gdk_pixbuf_unref(pixbuf);
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
nsWindow::Validate()
{
    
    
    if (!mDrawingarea)
        return NS_OK;

    GdkRegion *region = gdk_window_get_update_area(mDrawingarea->inner_window);

    if (region)
        gdk_region_destroy(region);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(PRBool aIsSynchronous)
{
    GdkRectangle rect;

    rect.x = mBounds.x;
    rect.y = mBounds.y;
    rect.width = mBounds.width;
    rect.height = mBounds.height;

    LOGDRAW(("Invalidate (all) [%p]: %d %d %d %d\n", (void *)this,
             rect.x, rect.y, rect.width, rect.height));

    if (!mDrawingarea)
        return NS_OK;

    gdk_window_invalidate_rect(mDrawingarea->inner_window,
                               &rect, FALSE);
    if (aIsSynchronous)
        gdk_window_process_updates(mDrawingarea->inner_window, FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::Invalidate(const nsRect &aRect,
                     PRBool        aIsSynchronous)
{
    GdkRectangle rect;

    rect.x = aRect.x;
    rect.y = aRect.y;
    rect.width = aRect.width;
    rect.height = aRect.height;

    LOGDRAW(("Invalidate (rect) [%p]: %d %d %d %d (sync: %d)\n", (void *)this,
             rect.x, rect.y, rect.width, rect.height, aIsSynchronous));

    if (!mDrawingarea)
        return NS_OK;

    gdk_window_invalidate_rect(mDrawingarea->inner_window,
                               &rect, FALSE);
    if (aIsSynchronous)
        gdk_window_process_updates(mDrawingarea->inner_window, FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::InvalidateRegion(const nsIRegion* aRegion,
                           PRBool           aIsSynchronous)
{
    GdkRegion *region = nsnull;
    aRegion->GetNativeRegion((void *&)region);

    if (region && mDrawingarea) {
        GdkRectangle rect;
        gdk_region_get_clipbox(region, &rect);

        LOGDRAW(("Invalidate (region) [%p]: %d %d %d %d (sync: %d)\n",
                 (void *)this,
                 rect.x, rect.y, rect.width, rect.height, aIsSynchronous));

        gdk_window_invalidate_region(mDrawingarea->inner_window,
                                     region, FALSE);
    }
    else {
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

    gdk_window_process_updates(mDrawingarea->inner_window, FALSE);
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

    moz_drawingarea_scroll(mDrawingarea, aDx, aDy);

    
    for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
        nsRect bounds;
        kid->GetBounds(bounds);
        bounds.x += aDx;
        bounds.y += aDy;
        NS_STATIC_CAST(nsBaseWidget*, kid)->SetBounds(bounds);
    }

    
    gdk_window_process_all_updates();
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScrollWidgets(PRInt32 aDx,
                        PRInt32 aDy)
{
    if (!mDrawingarea)
        return NS_OK;

    moz_drawingarea_scroll(mDrawingarea, aDx, aDy);
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

        return mDrawingarea->inner_window;
        break;
    }

    case NS_NATIVE_PLUGIN_PORT:
        return SetupPluginPort();
        break;

    case NS_NATIVE_DISPLAY:
        return GDK_DISPLAY();
        break;

    case NS_NATIVE_GRAPHIC: {
        NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
        return (void *)NS_STATIC_CAST(nsGTKToolkit *, mToolkit)->GetSharedGC();
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
nsWindow::SetBorderStyle(nsBorderStyle aBorderStyle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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
    nsCStringArray iconList;

    

    ResolveIconName(aIconSpec, NS_LITERAL_STRING(".xpm"),
                    getter_AddRefs(iconFile));
    if (iconFile) {
        iconFile->GetNativePath(path);
        iconList.AppendCString(path);
    }

    
    ResolveIconName(aIconSpec, NS_LITERAL_STRING("16.xpm"),
                    getter_AddRefs(iconFile));
    if (iconFile) {
        iconFile->GetNativePath(path);
        iconList.AppendCString(path);
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
    gint x = 0, y = 0;

    if (mContainer) {
        gdk_window_get_root_origin(GTK_WIDGET(mContainer)->window,
                                   &x, &y);
        LOG(("WidgetToScreen (container) %d %d\n", x, y));
    }
    else if (mDrawingarea) {
        gdk_window_get_origin(mDrawingarea->inner_window, &x, &y);
        LOG(("WidgetToScreen (drawing) %d %d\n", x, y));
    }

    aNewRect.x = x + aOldRect.x;
    aNewRect.y = y + aOldRect.y;
    aNewRect.width = aOldRect.width;
    aNewRect.height = aOldRect.height;

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect)
{
    return NS_ERROR_NOT_IMPLEMENTED;
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

    GtkWidget *widget =
        get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);

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
                              PRBool             aDoCapture,
                              PRBool             aConsumeRollupEvent)
{
    if (!mDrawingarea)
        return NS_OK;

    GtkWidget *widget =
        get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);

    LOG(("CaptureRollupEvents %p\n", (void *)this));

    if (aDoCapture) {
        gRollupListener = aListener;
        gRollupWindow = do_GetWeakReference(NS_STATIC_CAST(nsIWidget*,
                                                           this));
        
        if (!nsWindow::DragInProgress()) {
            gtk_grab_add(widget);
            GrabPointer();
            GrabKeyboard();
        }
    }
    else {
        if (!nsWindow::DragInProgress()) {
            ReleaseGrabs();
            gtk_grab_remove(widget);
        }
        gRollupListener = nsnull;
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

void
nsWindow::LoseFocus(void)
{
    
    
    memset(mKeyDownFlags, 0, sizeof(mKeyDownFlags));

    
    DispatchLostFocusEvent();

    LOGFOCUS(("  widget lost focus [%p]\n", (void *)this));
}

#ifdef DEBUG


#define CAPS_LOCK_IS_ON \
(gdk_keyboard_get_modifiers() & GDK_LOCK_MASK)

#define WANT_PAINT_FLASHING \
(debug_WantPaintFlashing() && CAPS_LOCK_IS_ON)

static GdkModifierType
gdk_keyboard_get_modifiers()
{
  GdkModifierType m = (GdkModifierType) 0;

  gdk_window_get_pointer(NULL, NULL, NULL, &m);

  return m;
}

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

gboolean
nsWindow::OnExposeEvent(GtkWidget *aWidget, GdkEventExpose *aEvent)
{
    if (mIsDestroyed) {
        LOG(("Expose event on destroyed window [%p] window %p\n",
             (void *)this, (void *)aEvent->window));
        return FALSE;
    }

    if (!mDrawingarea)
        return FALSE;

    
    if (aEvent->window != mDrawingarea->inner_window)
        return FALSE;

    static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

    nsCOMPtr<nsIRegion> updateRegion = do_CreateInstance(kRegionCID);
    if (!updateRegion)
        return FALSE;

    updateRegion->Init();

    GdkRectangle *rects;
    gint nrects;
    gdk_region_get_rectangles(aEvent->region, &rects, &nrects);
    LOGDRAW(("sending expose event [%p] %p 0x%lx (rects follow):\n",
             (void *)this, (void *)aEvent->window,
             GDK_WINDOW_XWINDOW(aEvent->window)));

    GdkRectangle *r;
    GdkRectangle *r_end = rects + nrects;
    for (r = rects; r < r_end; ++r) {
        updateRegion->Union(r->x, r->y, r->width, r->height);
        LOGDRAW(("\t%d %d %d %d\n", r->x, r->y, r->width, r->height));
    }

    nsCOMPtr<nsIRenderingContext> rc = getter_AddRefs(GetRenderingContext());

    PRBool translucent;
    GetWindowTranslucency(translucent);
    nsIntRect boundsRect;
    GdkPixmap* bufferPixmap = nsnull;
    nsRefPtr<gfxXlibSurface> bufferPixmapSurface;

    updateRegion->GetBoundingBox(&boundsRect.x, &boundsRect.y,
                                 &boundsRect.width, &boundsRect.height);

    
    nsRefPtr<gfxContext> ctx
        = (gfxContext*)rc->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
    ctx->Save();
    ctx->NewPath();
    if (translucent) {
        
        
        
        
        ctx->Rectangle(gfxRect(boundsRect.x, boundsRect.y,
                               boundsRect.width, boundsRect.height));
    } else {
        for (r = rects; r < r_end; ++r) {
            ctx->Rectangle(gfxRect(r->x, r->y, r->width, r->height));
        }
    }
    ctx->Clip();

    
    if (translucent) {
        ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
    } else {
#ifdef MOZ_ENABLE_GLITZ
        ctx->PushGroup(gfxASurface::CONTENT_COLOR);
#else 
        
        
        
        
        GdkDrawable* d = GDK_DRAWABLE(mDrawingarea->inner_window);
        gint depth = gdk_drawable_get_depth(d);
        bufferPixmap = gdk_pixmap_new(d, boundsRect.width, boundsRect.height, depth);
        if (bufferPixmap) {
            GdkVisual* visual = gdk_drawable_get_visual(GDK_DRAWABLE(bufferPixmap));
            Visual* XVisual = gdk_x11_visual_get_xvisual(visual);
            Display* display = gdk_x11_drawable_get_xdisplay(GDK_DRAWABLE(bufferPixmap));
            Drawable drawable = gdk_x11_drawable_get_xid(GDK_DRAWABLE(bufferPixmap));
            bufferPixmapSurface =
                new gfxXlibSurface(display, drawable, XVisual,
                                   gfxIntSize(boundsRect.width, boundsRect.height));
            if (bufferPixmapSurface) {
                bufferPixmapSurface->SetDeviceOffset(gfxPoint(-boundsRect.x, -boundsRect.y));
                nsCOMPtr<nsIRenderingContext> newRC;
                nsresult rv = GetDeviceContext()->
                    CreateRenderingContextInstance(*getter_AddRefs(newRC));
                if (NS_FAILED(rv)) {
                    bufferPixmapSurface = nsnull;
                } else {
                    rv = newRC->Init(GetDeviceContext(), bufferPixmapSurface);
                    if (NS_FAILED(rv)) {
                        bufferPixmapSurface = nsnull;
                    } else {
                        rc = newRC;
                    }
                }
            }
        }
#endif 
    }

#if 0
    
    
    
#ifdef DEBUG
    if (WANT_PAINT_FLASHING && aEvent->window)
        gdk_window_flash(aEvent->window, 1, 100, aEvent->region);
#endif
#endif

    nsPaintEvent event(PR_TRUE, NS_PAINT, this);
    event.refPoint.x = aEvent->area.x;
    event.refPoint.y = aEvent->area.y;
    event.rect = nsnull;
    event.region = updateRegion;
    event.renderingContext = rc;

    nsEventStatus status;
    DispatchEvent(&event, status);

    
    
    if (NS_LIKELY(!mIsDestroyed)) {
        if (status != nsEventStatus_eIgnore) {
            if (translucent) {
                nsRefPtr<gfxPattern> pattern = ctx->PopGroup();
                ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
                ctx->SetPattern(pattern);
                ctx->Paint();

                nsRefPtr<gfxImageSurface> img =
                    new gfxImageSurface(gfxIntSize(boundsRect.width, boundsRect.height),
                                        gfxImageSurface::ImageFormatA8);
                img->SetDeviceOffset(gfxPoint(-boundsRect.x, -boundsRect.y));
            
                nsRefPtr<gfxContext> imgCtx = new gfxContext(img);
                imgCtx->SetPattern(pattern);
                imgCtx->SetOperator(gfxContext::OPERATOR_SOURCE);
                imgCtx->Paint();
        
                UpdateTranslucentWindowAlphaInternal(nsRect(boundsRect.x, boundsRect.y,
                                                            boundsRect.width, boundsRect.height),
                                                     img->Data(), img->Stride());
            } else {
#ifdef MOZ_ENABLE_GLITZ
                ctx->PopGroupToSource();
                ctx->Paint();
#else 
                if (bufferPixmapSurface) {
                    ctx->SetSource(bufferPixmapSurface);
                    ctx->Paint();
                }
#endif 
            }
        } else {
            
            if (translucent) {
                ctx->PopGroup();
            } else {
#ifdef MOZ_ENABLE_GLITZ
                ctx->PopGroup();
#endif 
            }
        }

        if (bufferPixmap) {
            g_object_unref(G_OBJECT(bufferPixmap));
        }

        ctx->Restore();
    }

    g_free(rects);

    
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

    
    
    
    if (mIsTopLevel) {
        mPlaced = PR_TRUE;
        
        nsRect oldrect, newrect;
        WidgetToScreen(oldrect, newrect);
        mBounds.x = newrect.x;
        mBounds.y = newrect.y;
    }

    nsGUIEvent event(PR_TRUE, NS_MOVE, this);

    event.refPoint.x = aEvent->x;
    event.refPoint.y = aEvent->y;

    
    
    nsEventStatus status;
    DispatchEvent(&event, status);

    return FALSE;
}

void
nsWindow::OnSizeAllocate(GtkWidget *aWidget, GtkAllocation *aAllocation)
{
    LOG(("size_allocate [%p] %d %d %d %d\n",
         (void *)this, aAllocation->x, aAllocation->y,
         aAllocation->width, aAllocation->height));

    nsRect rect(aAllocation->x, aAllocation->y,
                aAllocation->width, aAllocation->height);

    ResizeTransparencyBitmap(rect.width, rect.height);

    mBounds.width = rect.width;
    mBounds.height = rect.height;

    if (!mDrawingarea)
        return;

    moz_drawingarea_resize (mDrawingarea, rect.width, rect.height);

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

    nsMouseEvent event(PR_TRUE, NS_MOUSE_ENTER, this, nsMouseEvent::eReal);

    event.refPoint.x = nscoord(aEvent->x);
    event.refPoint.y = nscoord(aEvent->y);

    event.time = aEvent->time;

    LOG(("OnEnterNotify: %p\n", (void *)this));

    nsEventStatus status;
    DispatchEvent(&event, status);
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

    LOG(("OnLeaveNotify: %p\n", (void *)this));

    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::OnMotionNotifyEvent(GtkWidget *aWidget, GdkEventMotion *aEvent)
{
    
    
    sIsDraggingOutOf = PR_FALSE;

    
    
    
    XEvent xevent;
    PRPackedBool synthEvent = PR_FALSE;
    while (XCheckWindowEvent(GDK_WINDOW_XDISPLAY(aEvent->window),
                             GDK_WINDOW_XWINDOW(aEvent->window),
                             ButtonMotionMask, &xevent)) {
        synthEvent = PR_TRUE;
    }

    
    if (gPluginFocusWindow && gPluginFocusWindow != this) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gPluginFocusWindow;
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }

    nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, this, nsMouseEvent::eReal);

    if (synthEvent) {
        event.refPoint.x = nscoord(xevent.xmotion.x);
        event.refPoint.y = nscoord(xevent.xmotion.y);

        event.isShift   = (xevent.xmotion.state & GDK_SHIFT_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isControl = (xevent.xmotion.state & GDK_CONTROL_MASK)
            ? PR_TRUE : PR_FALSE;
        event.isAlt     = (xevent.xmotion.state & GDK_MOD1_MASK)
            ? PR_TRUE : PR_FALSE;

        event.time = xevent.xmotion.time;
    }
    else {
        event.refPoint.x = nscoord(aEvent->x);
        event.refPoint.y = nscoord(aEvent->y);

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

void
nsWindow::OnButtonPressEvent(GtkWidget *aWidget, GdkEventButton *aEvent)
{
    nsEventStatus status;

    
    
    
    
    
    
    GdkEvent *peekedEvent = gdk_event_peek();
    if (peekedEvent) {
        GdkEventType type = peekedEvent->any.type;
        gdk_event_free(peekedEvent);
        if (type == GDK_2BUTTON_PRESS || type == GDK_3BUTTON_PRESS)
            return;
    }

    
    mLastButtonPressTime = aEvent->time;
    mLastButtonReleaseTime = 0;

    
    nsWindow *containerWindow;
    GetContainerWindow(&containerWindow);

    if (!gFocusWindow) {
        containerWindow->mActivatePending = PR_FALSE;
        DispatchActivateEvent();
    }
    if (check_for_rollup(aEvent->window, aEvent->x_root, aEvent->y_root,
                         PR_FALSE))
        return;

    PRUint16 domButton;
    switch (aEvent->button) {
    case 2:
        domButton = nsMouseEvent::eMiddleButton;
        break;
    case 3:
        domButton = nsMouseEvent::eRightButton;
        break;
    default:
        domButton = nsMouseEvent::eLeftButton;
        break;
    }

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_DOWN, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent);

    DispatchEvent(&event, status);

    
    if (domButton == nsMouseEvent::eRightButton &&
        NS_LIKELY(!mIsDestroyed)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, NS_CONTEXTMENU, this,
                                      nsMouseEvent::eReal);
        InitButtonEvent(contextMenuEvent, aEvent);
        DispatchEvent(&contextMenuEvent, status);
    }
}

void
nsWindow::OnButtonReleaseEvent(GtkWidget *aWidget, GdkEventButton *aEvent)
{
    PRUint16 domButton;
    mLastButtonReleaseTime = aEvent->time;

    switch (aEvent->button) {
    case 2:
        domButton = nsMouseEvent::eMiddleButton;
        break;
    case 3:
        domButton = nsMouseEvent::eRightButton;
        break;
        
    case 4:
    case 5:
        return;
        break;
        
    default:
        domButton = nsMouseEvent::eLeftButton;
        break;
    }

    nsMouseEvent event(PR_TRUE, NS_MOUSE_BUTTON_UP, this, nsMouseEvent::eReal);
    event.button = domButton;
    InitButtonEvent(event, aEvent);

    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::OnContainerFocusInEvent(GtkWidget *aWidget, GdkEventFocus *aEvent)
{
    LOGFOCUS(("OnContainerFocusInEvent [%p]\n", (void *)this));
    
    
    
    if (mContainerBlockFocus) {
        LOGFOCUS(("Container focus is blocked [%p]\n", (void *)this));
        return;
    }

    if (mIsTopLevel)
        mActivatePending = PR_TRUE;

    
    GtkWidget* top_window = nsnull;
    GetToplevelWidget(&top_window);
    if (top_window && (GTK_WIDGET_VISIBLE(top_window)))
        SetUrgencyHint(top_window, PR_FALSE);

    
    DispatchGotFocusEvent();

    
    
    
    if (mActivatePending) {
        mActivatePending = PR_FALSE;
        DispatchActivateEvent();
    }

    LOGFOCUS(("Events sent from focus in event [%p]\n", (void *)this));
}

void
nsWindow::OnContainerFocusOutEvent(GtkWidget *aWidget, GdkEventFocus *aEvent)
{
    LOGFOCUS(("OnContainerFocusOutEvent [%p]\n", (void *)this));

    
    if (gPluginFocusWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = gPluginFocusWindow;
        gPluginFocusWindow->LoseNonXEmbedPluginFocus();
    }

    
    
    if (!gFocusWindow)
        return;

    GdkWindow *tmpWindow;
    tmpWindow = (GdkWindow *)gFocusWindow->GetNativeData(NS_NATIVE_WINDOW);
    nsWindow *tmpnsWindow = get_window_for_gdk_window(tmpWindow);

    while (tmpWindow && tmpnsWindow) {
        
        if (tmpnsWindow == this)
            goto foundit;

        tmpWindow = gdk_window_get_parent(tmpWindow);
        if (!tmpWindow)
            break;

        tmpnsWindow = get_owning_window_for_gdk_window(tmpWindow);
    }

    LOGFOCUS(("The focus widget was not a child of this window [%p]\n",
              (void *)this));

    return;

 foundit:

    nsRefPtr<nsWindow> kungFuDeathGrip = gFocusWindow;
#ifdef USE_XIM
    gFocusWindow->IMELoseFocus();
#endif

    gFocusWindow->LoseFocus();

    
    
    if (mIsTopLevel && NS_LIKELY(!gFocusWindow->mIsDestroyed))
        gFocusWindow->DispatchDeactivateEvent();

    gFocusWindow = nsnull;

    mActivatePending = PR_FALSE;

    LOGFOCUS(("Done with container focus out [%p]\n", (void *)this));
}

gboolean
nsWindow::OnKeyPressEvent(GtkWidget *aWidget, GdkEventKey *aEvent)
{
    LOGFOCUS(("OnKeyPressEvent [%p]\n", (void *)this));

#ifdef USE_XIM
    
    
   LOGIM(("key press [%p]: composing %d val %d\n",
           (void *)this, IMEComposingWindow() != nsnull, aEvent->keyval));
   if (IMEFilterEvent(aEvent))
       return TRUE;
   LOGIM(("sending as regular key press event\n"));
#endif

    nsEventStatus status;

    
    if (aEvent->keyval == GDK_Tab && aEvent->state & GDK_CONTROL_MASK &&
        aEvent->state & GDK_MOD1_MASK) {
        return TRUE;
    }

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;

    
    
    
    
    

    PRBool isKeyDownCancelled = PR_FALSE;
    
    PRUint32 domVirtualKeyCode = GdkKeyCodeToDOMKeyCode(aEvent->keyval);

    if (!IsKeyDown(domVirtualKeyCode)) {
        SetKeyDownFlag(domVirtualKeyCode);

        
        nsKeyEvent downEvent(PR_TRUE, NS_KEY_DOWN, this);
        InitKeyEvent(downEvent, aEvent);
        DispatchEvent(&downEvent, status);
        if (NS_UNLIKELY(mIsDestroyed))
            return PR_TRUE;
        isKeyDownCancelled = (status == nsEventStatus_eConsumeNoDefault);
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
    nsKeyEvent event(PR_TRUE, NS_KEY_PRESS, this);
    InitKeyEvent(event, aEvent);
    if (isKeyDownCancelled) {
      
      event.flags |= NS_EVENT_FLAG_NO_DEFAULT;
    }
    event.charCode = nsConvertCharCodeToUnicode(aEvent);
    if (event.charCode) {
        event.keyCode = 0;
        
        
        
        

        if (event.isControl || event.isAlt || event.isMeta) {
           
           
           
           
           
           
           
           if (!event.isShift &&
               event.charCode >= GDK_A &&
               event.charCode <= GDK_Z)
            event.charCode = gdk_keyval_to_lower(event.charCode);

           
           
           
           if (!event.isControl && event.isShift &&
               (event.charCode < GDK_0 || event.charCode > GDK_9)) {
               GdkKeymapKey k = { aEvent->hardware_keycode, aEvent->group, 0 };
               guint savedKeyval = aEvent->keyval;
               aEvent->keyval = gdk_keymap_lookup_key(gdk_keymap_get_default(), &k);
               PRUint32 unshiftedCharCode = nsConvertCharCodeToUnicode(aEvent);
               if (unshiftedCharCode)
                   event.charCode = unshiftedCharCode;
               else
                   aEvent->keyval = savedKeyval;
           }
        }
    }

    
    
    if (is_context_menu_key(event)) {
        nsMouseEvent contextMenuEvent(PR_TRUE, 0, nsnull, nsMouseEvent::eReal);
        key_event_to_context_menu_event(&event, &contextMenuEvent);
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

gboolean
nsWindow::OnKeyReleaseEvent(GtkWidget *aWidget, GdkEventKey *aEvent)
{
    LOGFOCUS(("OnKeyReleaseEvent [%p]\n", (void *)this));

#ifdef USE_XIM
    if (IMEFilterEvent(aEvent))
        return TRUE;
#endif

    
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

void
nsWindow::OnScrollEvent(GtkWidget *aWidget, GdkEventScroll *aEvent)
{
    nsMouseScrollEvent event(PR_TRUE, NS_MOUSE_SCROLL, this);
    InitMouseScrollEvent(event, aEvent);

    
    if (check_for_rollup(aEvent->window, aEvent->x_root, aEvent->y_root,
                         PR_TRUE)) {
        return;
    }

    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsWindow::OnVisibilityNotifyEvent(GtkWidget *aWidget,
                                  GdkEventVisibility *aEvent)
{
    switch (aEvent->state) {
    case GDK_VISIBILITY_UNOBSCURED:
    case GDK_VISIBILITY_PARTIAL:
        mIsVisible = PR_TRUE;
        
        EnsureGrabs();
        break;
    default: 
        mIsVisible = PR_FALSE;
        break;
    }
}

void
nsWindow::OnWindowStateEvent(GtkWidget *aWidget, GdkEventWindowState *aEvent)
{
    LOG(("nsWindow::OnWindowStateEvent [%p] changed %d new_window_state %d\n",
         (void *)this, aEvent->changed_mask, aEvent->new_window_state));

    nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);

    
    
    if (aEvent->changed_mask &
        (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_MAXIMIZED) == 0) {
        return;
    }

    if (aEvent->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
        LOG(("\tIconified\n"));
        event.mSizeMode = nsSizeMode_Minimized;
        mSizeState = nsSizeMode_Minimized;
    }
    else if (aEvent->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
        LOG(("\tMaximized\n"));
        event.mSizeMode = nsSizeMode_Maximized;
        mSizeState = nsSizeMode_Maximized;
    }
    else {
        LOG(("\tNormal\n"));
        event.mSizeMode = nsSizeMode_Normal;
        mSizeState = nsSizeMode_Normal;
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

    if (!mDrawingarea || NS_UNLIKELY(mIsDestroyed))
        return;

    
    GList *children =
        gdk_window_peek_children(mDrawingarea->inner_window);
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

gboolean
nsWindow::OnDragMotionEvent(GtkWidget *aWidget,
                            GdkDragContext *aDragContext,
                            gint aX,
                            gint aY,
                            guint aTime,
                            gpointer aData)
{
    LOG(("nsWindow::OnDragMotionSignal\n"));

    if (mLastButtonReleaseTime) {
      
      
      GtkWidget *widget = gtk_grab_get_current();
      GdkEvent event;
      gboolean retval;
      memset(&event, 0, sizeof(event));
      event.type = GDK_BUTTON_RELEASE;
      event.button.time = mLastButtonReleaseTime;
      event.button.button = 1;
      mLastButtonReleaseTime = 0;
      if (widget) {
        g_signal_emit_by_name(widget, "button_release_event", &event, &retval);
        return TRUE;
      }
    }

    sIsDraggingOutOf = PR_FALSE;

    
    ResetDragMotionTimer(aWidget, aDragContext, aX, aY, aTime);

    
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
    nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

    
    
    nscoord retx = 0;
    nscoord rety = 0;

    GdkWindow *innerWindow = get_inner_gdk_window(aWidget->window, aX, aY,
                                                  &retx, &rety);
    nsRefPtr<nsWindow> innerMostWidget = get_window_for_gdk_window(innerWindow);

    if (!innerMostWidget)
        innerMostWidget = this;

    
    if (mLastDragMotionWindow) {
        
        if (mLastDragMotionWindow != innerMostWidget) {
            
            nsRefPtr<nsWindow> kungFuDeathGrip = mLastDragMotionWindow;
            mLastDragMotionWindow->OnDragLeave();
            
            innerMostWidget->OnDragEnter(retx, rety);
        }
    }
    else {
        
        

        innerMostWidget->OnDragEnter(retx, rety);
    }

    
    mLastDragMotionWindow = innerMostWidget;

    
    dragSessionGTK->TargetSetLastContext(aWidget, aDragContext, aTime);

    
    dragSessionGTK->TargetStartDragMotion();

    dragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, innerMostWidget,
                       nsMouseEvent::eReal);

    InitDragEvent(event);

    
    UpdateDragStatus(event, aDragContext, dragService);

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
    

    LOG(("nsWindow::OnDragLeaveSignal(%p)\n", this));

    sIsDraggingOutOf = PR_TRUE;

    
    ResetDragMotionTimer(0, 0, 0, 0, 0);

    
    
    
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
    LOG(("nsWindow::OnDragDropSignal\n"));

    
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
    nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

    nscoord retx = 0;
    nscoord rety = 0;

    GdkWindow *innerWindow = get_inner_gdk_window(aWidget->window, aX, aY,
                                                  &retx, &rety);
    nsRefPtr<nsWindow> innerMostWidget = get_window_for_gdk_window(innerWindow);

    
    dragSessionGTK->TargetSetLastContext(aWidget, aDragContext, aTime);

    if (!innerMostWidget)
        innerMostWidget = this;

    
    if (mLastDragMotionWindow) {
        
        if (mLastDragMotionWindow != innerMostWidget) {
            
            nsRefPtr<nsWindow> kungFuDeathGrip = mLastDragMotionWindow;
            mLastDragMotionWindow->OnDragLeave();
            
            innerMostWidget->OnDragEnter(retx, rety);
        }
    }
    else {
        
        
        innerMostWidget->OnDragEnter(retx, rety);
    }

    
    
    if (mDragLeaveTimer) {
        mDragLeaveTimer->Cancel();
        mDragLeaveTimer = 0;
    }

    
    mLastDragMotionWindow = innerMostWidget;

    
    
    

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, innerMostWidget,
                       nsMouseEvent::eReal);

    InitDragEvent(event);

    
    UpdateDragStatus(event, aDragContext, dragService);

    event.refPoint.x = retx;
    event.refPoint.y = rety;
    event.time = aTime;

    nsEventStatus status;
    innerMostWidget->DispatchEvent(&event, status);

    
    
    
    if (!innerMostWidget->mIsDestroyed) {
        event.message = NS_DRAGDROP_DROP;
        event.widget = innerMostWidget;
        event.refPoint.x = retx;
        event.refPoint.y = rety;

        innerMostWidget->DispatchEvent(&event, status);
    }

    

    gdk_drop_finish(aDragContext, TRUE, aTime);

    
    
    
    
    dragSessionGTK->TargetSetLastContext(0, 0, 0);

    
    innerMostWidget->OnDragLeave();
    
    mLastDragMotionWindow = 0;

    
    
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
    LOG(("nsWindow::OnDragDataReceived(%p)\n", this));

    
    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
    nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

    dragSessionGTK->TargetDataReceived(aWidget, aDragContext, aX, aY,
                                       aSelectionData, aInfo, aTime);
}

void
nsWindow::OnDragLeave(void)
{
    LOG(("nsWindow::OnDragLeave(%p)\n", this));

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_EXIT, this, nsMouseEvent::eReal);

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
    

    LOG(("nsWindow::OnDragEnter(%p)\n", this));

    nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);

    if (dragService) {
        
        dragService->StartDragSession();
    }

    nsMouseEvent event(PR_TRUE, NS_DRAGDROP_ENTER, this, nsMouseEvent::eReal);

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

    NS_ASSERTION(aInitData->mWindowType != eWindowType_popup ||
                 !aParent, "Popups should not be hooked into nsIWidget hierarchy");

    
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

    
    MozDrawingarea *parentArea = nsnull;
    MozContainer   *parentMozContainer = nsnull;
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
        
        gpointer user_data = nsnull;
        user_data = g_object_get_data(G_OBJECT(parentGdkWindow),
                                      "mozdrawingarea");
        parentArea = MOZ_DRAWINGAREA(user_data);

        NS_ASSERTION(parentArea, "no drawingarea for parent widget!\n");
        if (!parentArea)
            return NS_ERROR_FAILURE;

        
        user_data = nsnull;
        gdk_window_get_user_data(parentArea->inner_window, &user_data);
        NS_ASSERTION(user_data, "no user data for parentArea\n");
        if (!user_data)
            return NS_ERROR_FAILURE;

        
        parentMozContainer = MOZ_CONTAINER(user_data);
        NS_ASSERTION(parentMozContainer,
                     "owning widget is not a mozcontainer!\n");
        if (!parentMozContainer)
            return NS_ERROR_FAILURE;

        
        
        topLevelParent =
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(parentMozContainer)));
    }

    GdkVisual* visual = nsnull;
#ifdef MOZ_ENABLE_GLITZ
    if (gfxPlatform::UseGlitz()) {
        nsCOMPtr<nsIDeviceContext> dc = aContext;
        if (!dc) {
            nsCOMPtr<nsIDeviceContext> dc = do_CreateInstance(kDeviceContextCID);
            
            dc->Init(nsnull);
        }

        Display* dpy = GDK_DISPLAY();
        int defaultScreen = gdk_x11_get_default_screen();
        glitz_drawable_format_t* format = glitz_glx_find_window_format (dpy, defaultScreen,
                                                                        0, NULL, 0);
        if (format) {
            XVisualInfo* vinfo = glitz_glx_get_visual_info_from_format(dpy, defaultScreen, format);
            GdkScreen* screen = gdk_display_get_screen(gdk_x11_lookup_xdisplay(dpy), defaultScreen);
            visual = gdk_x11_screen_lookup_visual(screen, vinfo->visualid);
        } else {
            
            gfxPlatform::SetUseGlitz(PR_FALSE);
        }
    }
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
            if (parentArea) {
                nsWindow *parentnsWindow =
                    get_window_for_gdk_window(parentArea->inner_window);
                NS_ASSERTION(parentnsWindow,
                             "no nsWindow for parentArea!");
                if (parentnsWindow && parentnsWindow->mWindowGroup) {
                    gtk_window_group_add_window(parentnsWindow->mWindowGroup,
                                                GTK_WINDOW(mShell));
                    
                    mWindowGroup = parentnsWindow->mWindowGroup;
                    LOG(("adding window %p to group %p\n",
                         (void *)mShell, (void *)mWindowGroup));
                }
            }
        }
        else if (mWindowType == eWindowType_popup) {
            mShell = gtk_window_new(GTK_WINDOW_POPUP);
            gtk_window_set_wmclass(GTK_WINDOW(mShell), "Popup", cBrand.get());

            if (topLevelParent) {
                gtk_window_set_transient_for(GTK_WINDOW(mShell),
                                            topLevelParent);
                mTransientParent = topLevelParent;

                if (topLevelParent->group) {
                    gtk_window_group_add_window(topLevelParent->group,
                                            GTK_WINDOW(mShell));
                    mWindowGroup = topLevelParent->group;
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

        
        mContainer = MOZ_CONTAINER(moz_container_new());
        gtk_container_add(GTK_CONTAINER(mShell), GTK_WIDGET(mContainer));
        gtk_widget_realize(GTK_WIDGET(mContainer));

        
        gtk_window_set_focus(GTK_WINDOW(mShell), GTK_WIDGET(mContainer));

        
        mDrawingarea = moz_drawingarea_new(nsnull, mContainer, visual);

        if (mWindowType == eWindowType_popup) {
            
            

            mCursor = eCursor_wait; 
                                    
                                    
                                    
            SetCursor(eCursor_standard);
        }
    }
        break;
    case eWindowType_child: {
        if (parentMozContainer) {
            mDrawingarea = moz_drawingarea_new(parentArea, parentMozContainer, visual);
        }
        else if (parentGtkContainer) {
            mContainer = MOZ_CONTAINER(moz_container_new());
            gtk_container_add(parentGtkContainer, GTK_WIDGET(mContainer));
            gtk_widget_realize(GTK_WIDGET(mContainer));

            mDrawingarea = moz_drawingarea_new(nsnull, mContainer, visual);
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
    
    
    if (mContainer)
        gtk_widget_set_double_buffered (GTK_WIDGET(mContainer),FALSE);

    
    
    g_object_set_data(G_OBJECT(mDrawingarea->clip_window), "nsWindow",
                      this);
    g_object_set_data(G_OBJECT(mDrawingarea->inner_window), "nsWindow",
                      this);

    g_object_set_data(G_OBJECT(mDrawingarea->clip_window), "mozdrawingarea",
                      mDrawingarea);
    g_object_set_data(G_OBJECT(mDrawingarea->inner_window), "mozdrawingarea",
                      mDrawingarea);

    if (mContainer)
        g_object_set_data(G_OBJECT(mContainer), "nsWindow", this);

    if (mShell)
        g_object_set_data(G_OBJECT(mShell), "nsWindow", this);

    
    if (mShell) {
        g_signal_connect(G_OBJECT(mShell), "configure_event",
                         G_CALLBACK(configure_event_cb), NULL);
        g_signal_connect(G_OBJECT(mShell), "delete_event",
                         G_CALLBACK(delete_event_cb), NULL);
        g_signal_connect(G_OBJECT(mShell), "window_state_event",
                         G_CALLBACK(window_state_event_cb), NULL);

        GtkSettings* default_settings = gtk_settings_get_default();
        g_signal_connect_after(default_settings,
                               "notify::gtk-theme-name",
                               G_CALLBACK(theme_changed_cb), this);
        g_signal_connect_after(default_settings,
                               "notify::gtk-font-name",
                               G_CALLBACK(theme_changed_cb), this);
    }

    if (mContainer) {
        g_signal_connect_after(G_OBJECT(mContainer), "size_allocate",
                               G_CALLBACK(size_allocate_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "expose_event",
                         G_CALLBACK(expose_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "enter_notify_event",
                         G_CALLBACK(enter_notify_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "leave_notify_event",
                         G_CALLBACK(leave_notify_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "motion_notify_event",
                         G_CALLBACK(motion_notify_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "button_press_event",
                         G_CALLBACK(button_press_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "button_release_event",
                         G_CALLBACK(button_release_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "focus_in_event",
                         G_CALLBACK(focus_in_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "focus_out_event",
                         G_CALLBACK(focus_out_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "key_press_event",
                         G_CALLBACK(key_press_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "key_release_event",
                         G_CALLBACK(key_release_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "scroll_event",
                         G_CALLBACK(scroll_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "visibility_notify_event",
                         G_CALLBACK(visibility_notify_event_cb), NULL);

        gtk_drag_dest_set((GtkWidget *)mContainer,
                          (GtkDestDefaults)0,
                          NULL,
                          0,
                          (GdkDragAction)0);

        g_signal_connect(G_OBJECT(mContainer), "drag_motion",
                         G_CALLBACK(drag_motion_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "drag_leave",
                         G_CALLBACK(drag_leave_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "drag_drop",
                         G_CALLBACK(drag_drop_event_cb), NULL);
        g_signal_connect(G_OBJECT(mContainer), "drag_data_received",
                         G_CALLBACK(drag_data_received_event_cb), NULL);

#ifdef USE_XIM
        
        
        if (mWindowType != eWindowType_popup)
            IMECreateContext();
#endif
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

    if (mDrawingarea) {
        LOG(("\tmDrawingarea %p %p %p %lx %lx\n", (void *)mDrawingarea,
             (void *)mDrawingarea->clip_window,
             (void *)mDrawingarea->inner_window,
             GDK_WINDOW_XWINDOW(mDrawingarea->clip_window),
             GDK_WINDOW_XWINDOW(mDrawingarea->inner_window)));
    }

    
    if (!mIsTopLevel)
        Resize(mBounds.x, mBounds.y, mBounds.width, mBounds.height, PR_FALSE);

#ifdef ACCESSIBILITY
    nsresult rv;
    if (!sAccessibilityChecked) {
        sAccessibilityChecked = PR_TRUE;

        
        const char *envValue = PR_GetEnv(sAccEnv);
        if (envValue) {
            sAccessibilityEnabled = atoi(envValue);
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

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetWindowClass(const nsAString &xulWinType)
{
  if (!mShell)
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

  gdk_window_set_role(GTK_WIDGET(mShell)->window, role);
  
  
  XSetClassHint(GDK_DISPLAY(),
                GDK_WINDOW_XWINDOW(GTK_WIDGET(mShell)->window),
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

    if (mIsTopLevel) {
        gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
    }
    else if (mContainer) {
        GtkAllocation allocation;
        allocation.x = 0;
        allocation.y = 0;
        allocation.width = aWidth;
        allocation.height = aHeight;
        gtk_widget_size_allocate(GTK_WIDGET(mContainer), &allocation);
    }

    moz_drawingarea_resize (mDrawingarea, aWidth, aHeight);
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
        if (mParent && mWindowType == eWindowType_popup) {
            nsRect oldrect, newrect;
            oldrect.x = aX;
            oldrect.y = aY;
            mParent->WidgetToScreen(oldrect, newrect);
            moz_drawingarea_resize(mDrawingarea, aWidth, aHeight);
            gtk_window_move(GTK_WINDOW(mShell), newrect.x, newrect.y);
            gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
        }
        else {
            
            
            
            
            if (mPlaced)
                gtk_window_move(GTK_WINDOW(mShell), aX, aY);

            gtk_window_resize(GTK_WINDOW(mShell), aWidth, aHeight);
            moz_drawingarea_resize(mDrawingarea, aWidth, aHeight);
        }
    }
    else if (mContainer) {
        GtkAllocation allocation;
        allocation.x = 0;
        allocation.y = 0;
        allocation.width = aWidth;
        allocation.height = aHeight;
        gtk_widget_size_allocate(GTK_WIDGET(mContainer), &allocation);
        moz_drawingarea_move_resize(mDrawingarea, aX, aY, aWidth, aHeight);
    }
    else if (mDrawingarea) {
        moz_drawingarea_move_resize(mDrawingarea, aX, aY, aWidth, aHeight);
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

            moz_drawingarea_set_visibility(mDrawingarea, aAction);
            gtk_widget_show(GTK_WIDGET(mContainer));
            gtk_widget_show(mShell);
        }
        else if (mContainer) {
            moz_drawingarea_set_visibility(mDrawingarea, TRUE);
            gtk_widget_show(GTK_WIDGET(mContainer));
        }
        else if (mDrawingarea) {
            moz_drawingarea_set_visibility(mDrawingarea, TRUE);
        }
    }
    else {
        if (mIsTopLevel) {
            gtk_widget_hide(GTK_WIDGET(mShell));
            gtk_widget_hide(GTK_WIDGET(mContainer));
        }
        else if (mContainer) {
            gtk_widget_hide(GTK_WIDGET(mContainer));
            moz_drawingarea_set_visibility(mDrawingarea, FALSE);
        }
        if (mDrawingarea) {
            moz_drawingarea_set_visibility(mDrawingarea, FALSE);
        }
    }
}

void
nsWindow::EnsureGrabs(void)
{
    if (mRetryPointerGrab)
        GrabPointer();
    if (mRetryKeyboardGrab)
        GrabKeyboard();
}

#ifndef MOZ_XUL
void
nsWindow::ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight)
{
}

void
nsWindow::ApplyTransparencyBitmap()
{
}
#else
NS_IMETHODIMP
nsWindow::SetWindowTranslucency(PRBool aTranslucent)
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
        if (!topWidget)
            return NS_ERROR_FAILURE;

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow)
            return NS_ERROR_FAILURE;

        return topWindow->SetWindowTranslucency(aTranslucent);
    }

    if (mIsTranslucent == aTranslucent)
        return NS_OK;

    if (!aTranslucent) {
        if (mTransparencyBitmap) {
            delete[] mTransparencyBitmap;
            mTransparencyBitmap = nsnull;
            mTransparencyBitmapWidth = 0;
            mTransparencyBitmapHeight = 0;
            gtk_widget_reset_shapes(mShell);
        }
    } 
    

    mIsTranslucent = aTranslucent;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetWindowTranslucency(PRBool& aTranslucent)
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
        if (!topWidget) {
            aTranslucent = PR_FALSE;
            return NS_ERROR_FAILURE;
        }

        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        if (!topWindow) {
            aTranslucent = PR_FALSE;
            return NS_ERROR_FAILURE;
        }

        return topWindow->GetWindowTranslucency(aTranslucent);
    }

    aTranslucent = mIsTranslucent;
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
        const nsRect& aRect, PRUint8* aAlphas, PRInt32 aStride)
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
        const nsRect& aRect, PRUint8* aAlphas, PRInt32 aStride)
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
    gtk_widget_reset_shapes(mShell);
    GdkBitmap* maskBitmap = gdk_bitmap_create_from_data(mShell->window,
            mTransparencyBitmap,
            mTransparencyBitmapWidth, mTransparencyBitmapHeight);
    if (!maskBitmap)
        return;

    gtk_widget_shape_combine_mask(mShell, maskBitmap, 0, 0);
    gdk_bitmap_unref(maskBitmap);
}

nsresult
nsWindow::UpdateTranslucentWindowAlphaInternal(const nsRect& aRect,
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

    NS_ASSERTION(mIsTranslucent, "Window is not transparent");

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

NS_IMETHODIMP
nsWindow::UpdateTranslucentWindowAlpha(const nsRect& aRect, PRUint8* aAlphas)
{
    return UpdateTranslucentWindowAlphaInternal(aRect, aAlphas, aRect.width);
}
#endif

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

    gint retval;
    retval = gdk_pointer_grab(mDrawingarea->inner_window, TRUE,
                              (GdkEventMask)(GDK_BUTTON_PRESS_MASK |
                                             GDK_BUTTON_RELEASE_MASK |
                                             GDK_ENTER_NOTIFY_MASK |
                                             GDK_LEAVE_NOTIFY_MASK |
                                             GDK_POINTER_MOTION_MASK),
                              (GdkWindow *)NULL, NULL, GDK_CURRENT_TIME);

    if (retval != GDK_GRAB_SUCCESS) {
        LOG(("GrabPointer: pointer grab failed\n"));
        mRetryPointerGrab = PR_TRUE;
    }
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

    
    
    
    GdkWindow *grabWindow;

    if (mTransientParent)
        grabWindow = GTK_WIDGET(mTransientParent)->window;
    else if (mDrawingarea)
        grabWindow = mDrawingarea->inner_window;
    else
        return;

    gint retval;
    retval = gdk_keyboard_grab(grabWindow, TRUE, GDK_CURRENT_TIME);

    if (retval != GDK_GRAB_SUCCESS) {
        LOG(("GrabKeyboard: keyboard grab failed %d\n", retval));
        gdk_pointer_ungrab(GDK_CURRENT_TIME);
        mRetryKeyboardGrab = PR_TRUE;
    }
}

void
nsWindow::ReleaseGrabs(void)
{
    LOG(("ReleaseGrabs\n"));

    mRetryPointerGrab = PR_FALSE;
    mRetryKeyboardGrab = PR_FALSE;

    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    gdk_keyboard_ungrab(GDK_CURRENT_TIME);
}

void
nsWindow::GetToplevelWidget(GtkWidget **aWidget)
{
    *aWidget = nsnull;

    if (mShell) {
        *aWidget = mShell;
        return;
    }

    if (!mDrawingarea)
        return;

    GtkWidget *widget =
        get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);
    if (!widget)
        return;

    *aWidget = gtk_widget_get_toplevel(widget);
}

void
nsWindow::GetContainerWindow(nsWindow **aWindow)
{
    if (!mDrawingarea)
        return;

    GtkWidget *owningWidget =
        get_gtk_widget_for_gdk_window(mDrawingarea->inner_window);

    *aWindow = get_window_for_gtk_widget(owningWidget);
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
    if (!mDrawingarea)
        return nsnull;

    if (GDK_WINDOW_OBJECT(mDrawingarea->inner_window)->destroyed == TRUE)
        return nsnull;

    
    
    
    XWindowAttributes xattrs;
    XGetWindowAttributes(GDK_DISPLAY (),
                         GDK_WINDOW_XWINDOW(mDrawingarea->inner_window),
                         &xattrs);
    XSelectInput (GDK_DISPLAY (),
                  GDK_WINDOW_XWINDOW(mDrawingarea->inner_window),
                  xattrs.your_event_mask |
                  SubstructureNotifyMask);

    gdk_window_add_filter(mDrawingarea->inner_window,
                          plugin_window_filter_func,
                          this);

    XSync(GDK_DISPLAY(), False);

    return (void *)GDK_WINDOW_XWINDOW(mDrawingarea->inner_window);
}

nsresult
nsWindow::SetWindowIconList(const nsCStringArray &aIconList)
{
    GList *list = NULL;

    for (int i = 0; i < aIconList.Count(); ++i) {
        const char *path = aIconList[i]->get();
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
    
    nsCOMPtr<nsILocalFile> iconFile;
    ResolveIconName(NS_LITERAL_STRING("default"),
                    NS_LITERAL_STRING(".xpm"),
                    getter_AddRefs(iconFile));
    if (!iconFile) {
        NS_WARNING("default.xpm not found");
        return;
    }

    nsCAutoString path;
    iconFile->GetNativePath(path);

    nsCStringArray iconList;
    iconList.AppendCString(path);

    SetWindowIconList(iconList);
}

void
nsWindow::SetPluginType(PluginType aPluginType)
{
    mPluginType = aPluginType;
}

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

    XGetInputFocus(GDK_WINDOW_XDISPLAY(mDrawingarea->inner_window),
                   &curFocusWindow,
                   &focusState);

    LOGFOCUS(("\t curFocusWindow=%p\n", curFocusWindow));

    GdkWindow* toplevel = gdk_window_get_toplevel
                                (mDrawingarea->inner_window);
    GdkWindow *gdkfocuswin = gdk_window_lookup(curFocusWindow);

    
    
    
    if (gdkfocuswin != toplevel) {
        return;
    }

    
    mOldFocusWindow = curFocusWindow;
    XRaiseWindow(GDK_WINDOW_XDISPLAY(mDrawingarea->inner_window),
                 GDK_WINDOW_XWINDOW(mDrawingarea->inner_window));
    gdk_error_trap_push();
    XSetInputFocus(GDK_WINDOW_XDISPLAY(mDrawingarea->inner_window),
                   GDK_WINDOW_XWINDOW(mDrawingarea->inner_window),
                   RevertToNone,
                   CurrentTime);
    gdk_flush();
    gdk_error_trap_pop();
    gPluginFocusWindow = this;
    gdk_window_add_filter(NULL, plugin_client_message_filter, this);

    LOGFOCUS(("nsWindow::SetNonXEmbedPluginFocus oldfocus=%p new=%p\n",
                mOldFocusWindow,
                GDK_WINDOW_XWINDOW(mDrawingarea->inner_window)));
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

    XGetInputFocus(GDK_WINDOW_XDISPLAY(mDrawingarea->inner_window),
                   &curFocusWindow,
                   &focusState);

    
    
    
    
    if (!curFocusWindow ||
        curFocusWindow == GDK_WINDOW_XWINDOW(mDrawingarea->inner_window)) {

        gdk_error_trap_push();
        XRaiseWindow(GDK_WINDOW_XDISPLAY(mDrawingarea->inner_window),
                     mOldFocusWindow);
        XSetInputFocus(GDK_WINDOW_XDISPLAY(mDrawingarea->inner_window),
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
    if (aStyle & eBorderStyle_close) {
#ifdef DEBUG
        printf("we don't handle eBorderStyle_close yet... please fix me\n");
#endif 
    }

    return w;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(PRBool aFullScreen)
{
#if GTK_CHECK_VERSION(2,2,0)
    if (aFullScreen)
        gdk_window_fullscreen (mShell->window);
    else
        gdk_window_unfullscreen (mShell->window);
#else
    HideWindowChrome(aFullScreen);
#endif
    return MakeFullScreenInternal(aFullScreen);
}

NS_IMETHODIMP
nsWindow::HideWindowChrome(PRBool aShouldHide)
{
    if (!mShell) {
        
        GtkWidget *topWidget = nsnull;
        GetToplevelWidget(&topWidget);
        nsWindow *topWindow = get_window_for_gtk_widget(topWidget);
        return topWindow->HideWindowChrome(aShouldHide);
    }

    
    
    
    gdk_window_hide(mShell->window);

    gint wmd;
    if (aShouldHide)
        wmd = 0;
    else
        wmd = ConvertBorderStyles(mBorderStyle);

    gdk_window_set_decorations(mShell->window, (GdkWMDecoration) wmd);

    gdk_window_show(mShell->window);

    
    
    
    
    
    XSync(GDK_DISPLAY(), False);

    return NS_OK;
}

PRBool
check_for_rollup(GdkWindow *aWindow, gdouble aMouseX, gdouble aMouseY,
                 PRBool aIsWheel)
{
    PRBool retVal = PR_FALSE;
    nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWindow);

    if (rollupWidget && gRollupListener) {
        GdkWindow *currentPopup =
            (GdkWindow *)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);
        if (!is_mouse_in_window(currentPopup, aMouseX, aMouseY)) {
            PRBool rollup = PR_TRUE;
            if (aIsWheel) {
                gRollupListener->ShouldRollupOnMouseWheelEvent(&rollup);
                retVal = PR_TRUE;
            }
            
            
            
            nsCOMPtr<nsIMenuRollup> menuRollup;
            menuRollup = (do_QueryInterface(gRollupListener));
            if (menuRollup) {
                nsCOMPtr<nsISupportsArray> widgetChain;
                menuRollup->GetSubmenuWidgetChain(getter_AddRefs(widgetChain));
                if (widgetChain) {
                    PRUint32 count = 0;
                    widgetChain->Count(&count);
                    for (PRUint32 i=0; i<count; ++i) {
                        nsCOMPtr<nsISupports> genericWidget;
                        widgetChain->GetElementAt(i,
                                                  getter_AddRefs(genericWidget));
                        nsCOMPtr<nsIWidget> widget(do_QueryInterface(genericWidget));
                        if (widget) {
                            GdkWindow* currWindow =
                                (GdkWindow*) widget->GetNativeData(NS_NATIVE_WINDOW);
                            if (is_mouse_in_window(currWindow, aMouseX, aMouseY)) {
                                rollup = PR_FALSE;
                                break;
                            }
                        }
                    } 
                }
            } 

            
            if (rollup) {
                gRollupListener->Rollup();
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
nsWindow::DragInProgress(void)
{
    
    
    
    return (mLastDragMotionWindow || sIsDraggingOutOf);
}


PRBool
is_mouse_in_window (GdkWindow* aWindow, gdouble aMouseX, gdouble aMouseY)
{
    gint x = 0;
    gint y = 0;
    gint w, h;

    gint offsetX = 0;
    gint offsetY = 0;

    GtkWidget *widget;
    GdkWindow *window;

    window = aWindow;

    while (window) {
        gint tmpX = 0;
        gint tmpY = 0;

        gdk_window_get_position(window, &tmpX, &tmpY);
        widget = get_gtk_widget_for_gdk_window(window);

        
        
        if (GTK_IS_WINDOW(widget)) {
            x = tmpX + offsetX;
            y = tmpY + offsetY;
            break;
        }

        offsetX += tmpX;
        offsetY += tmpY;
        window = gdk_window_get_parent(window);
    }

    gdk_window_get_size(aWindow, &w, &h);

    if (aMouseX > x && aMouseX < x + w &&
        aMouseY > y && aMouseY < y + h)
        return PR_TRUE;

    return PR_FALSE;
}


nsWindow *
get_window_for_gtk_widget(GtkWidget *widget)
{
    gpointer user_data;
    user_data = g_object_get_data(G_OBJECT(widget), "nsWindow");

    if (!user_data)
        return nsnull;

    return NS_STATIC_CAST(nsWindow *, user_data);
}


nsWindow *
get_window_for_gdk_window(GdkWindow *window)
{
    gpointer user_data;
    user_data = g_object_get_data(G_OBJECT(window), "nsWindow");

    if (!user_data)
        return nsnull;

    return NS_STATIC_CAST(nsWindow *, user_data);
}


nsWindow *
get_owning_window_for_gdk_window(GdkWindow *window)
{
    GtkWidget *owningWidget = get_gtk_widget_for_gdk_window(window);
    if (!owningWidget)
        return nsnull;

    gpointer user_data;
    user_data = g_object_get_data(G_OBJECT(owningWidget), "nsWindow");

    if (!user_data)
        return nsnull;

    return (nsWindow *)user_data;
}


GtkWidget *
get_gtk_widget_for_gdk_window(GdkWindow *window)
{
    gpointer user_data = NULL;
    gdk_window_get_user_data(window, &user_data);
    if (!user_data)
        return NULL;

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
        newType = MOZ_CURSOR_QUESTION_ARROW;
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
    case eCursor_col_resize:
        newType = MOZ_CURSOR_COL_RESIZE;
        break;
    case eCursor_row_resize:
        newType = MOZ_CURSOR_ROW_RESIZE;
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
        gdkcursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
        break;
    case eCursor_ew_resize:
        gdkcursor = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
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
        mask =
            gdk_bitmap_create_from_data(NULL,
                                        (char *)GtkCursors[newType].mask_bits,
                                        32, 32);

        gdkcursor = gdk_cursor_new_from_pixmap(cursor, mask, &fg, &bg,
                                               GtkCursors[newType].hot_x,
                                               GtkCursors[newType].hot_y);

        gdk_bitmap_unref(mask);
        gdk_bitmap_unref(cursor);
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
    if (is_parent_ungrab_enter(event)) {
        return TRUE;
    }

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

    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

    window->OnLeaveNotifyEvent(widget, event);

    return TRUE;
}


gboolean
motion_notify_event_cb(GtkWidget *widget, GdkEventMotion *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

    window->OnMotionNotifyEvent(widget, event);

    return TRUE;
}


gboolean
button_press_event_cb(GtkWidget *widget, GdkEventButton *event)
{
    LOG(("button_press_event_cb\n"));
    nsWindow *window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

    window->OnButtonPressEvent(widget, event);

    return TRUE;
}


gboolean
button_release_event_cb(GtkWidget *widget, GdkEventButton *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
    if (!window)
        return TRUE;

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


GdkFilterReturn
plugin_window_filter_func(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
    GtkWidget *widget;
    GdkWindow *plugin_window;
    gpointer  user_data;
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
                user_data = nsnull;
                gdk_window_get_user_data(plugin_window, &user_data);
                widget = GTK_WIDGET(user_data);

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

    
    
    
    
    Display *dpy ;
    dpy = GDK_WINDOW_XDISPLAY((GdkWindow*)(gPluginFocusWindow->
                GetNativeData(NS_NATIVE_WINDOW)));
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


gboolean
key_press_event_cb(GtkWidget *widget, GdkEventKey *event)
{
    LOG(("key_press_event_cb\n"));
    
    nsWindow *window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    nsRefPtr<nsWindow> focusWindow = gFocusWindow ? gFocusWindow : window;

    return focusWindow->OnKeyPressEvent(widget, event);
}

gboolean
key_release_event_cb(GtkWidget *widget, GdkEventKey *event)
{
    LOG(("key_release_event_cb\n"));
    
    nsWindow *window = get_window_for_gtk_widget(widget);
    if (!window)
        return FALSE;

    nsRefPtr<nsWindow> focusWindow = gFocusWindow ? gFocusWindow : window;

    return focusWindow->OnKeyReleaseEvent(widget, event);
}


gboolean
scroll_event_cb(GtkWidget *widget, GdkEventScroll *event)
{
    nsRefPtr<nsWindow> window = get_window_for_gdk_window(event->window);
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
nsWindow::InitDragEvent(nsMouseEvent &aEvent)
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
nsWindow::UpdateDragStatus(nsMouseEvent   &aEvent,
                           GdkDragContext *aDragContext,
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
    if (prefs) {
        PRBool val = PR_TRUE;
        nsresult rv;
        rv = prefs->GetBoolPref("mozilla.widget.raise-on-setfocus", &val);
        if (NS_SUCCEEDED(rv))
            gRaiseWindows = val;
    }

    return NS_OK;
}

void
nsWindow::ResetDragMotionTimer(GtkWidget *aWidget,
                               GdkDragContext *aDragContext,
                               gint aX, gint aY, guint aTime)
{

    
    
    
    if (aWidget)
        gtk_widget_ref(aWidget);
    if (mDragMotionWidget)
        gtk_widget_unref(mDragMotionWidget);
    mDragMotionWidget = aWidget;

    if (aDragContext)
        gdk_drag_context_ref(aDragContext);
    if (mDragMotionContext)
        gdk_drag_context_unref(mDragMotionContext);
    mDragMotionContext = aDragContext;

    mDragMotionX = aX;
    mDragMotionY = aY;
    mDragMotionTime = aTime;

    
    if (mDragMotionTimerID) {
        gtk_timeout_remove(mDragMotionTimerID);
        mDragMotionTimerID = 0;
        LOG(("*** canceled motion timer\n"));
    }

    
    
    if (!aWidget) {
        return;
    }

    
    mDragMotionTimerID = gtk_timeout_add(100,
                                         (GtkFunction)DragMotionTimerCallback,
                                         this);
}

void
nsWindow::FireDragMotionTimer(void)
{
    LOG(("nsWindow::FireDragMotionTimer(%p)\n", this));

    OnDragMotionEvent(mDragMotionWidget, mDragMotionContext,
                      mDragMotionX, mDragMotionY, mDragMotionTime,
                      this);
}

void
nsWindow::FireDragLeaveTimer(void)
{
    LOG(("nsWindow::FireDragLeaveTimer(%p)\n", this));

    mDragLeaveTimer = 0;

    
    if (mLastDragMotionWindow) {
        nsRefPtr<nsWindow> kungFuDeathGrip = mLastDragMotionWindow;
        
        mLastDragMotionWindow->OnDragLeave();
        mLastDragMotionWindow = 0;
    }
}


guint
nsWindow::DragMotionTimerCallback(gpointer aClosure)
{
    nsRefPtr<nsWindow> window = NS_STATIC_CAST(nsWindow *, aClosure);
    window->FireDragMotionTimer();
    return FALSE;
}


void
nsWindow::DragLeaveTimerCallback(nsITimer *aTimer, void *aClosure)
{
    nsRefPtr<nsWindow> window = NS_STATIC_CAST(nsWindow *, aClosure);
    window->FireDragLeaveTimer();
}


GdkWindow *
get_inner_gdk_window (GdkWindow *aWindow,
                      gint x, gint y,
                      gint *retx, gint *rety)
{
    gint cx, cy, cw, ch, cd;
    GList * children = gdk_window_peek_children(aWindow);
    guint num = g_list_length(children);
    for (int i = 0; i < (int)num; i++) {
        GList * child = g_list_nth(children, num - i - 1) ;
        if (child) {
            GdkWindow * childWindow = (GdkWindow *) child->data;
            gdk_window_get_geometry (childWindow, &cx, &cy, &cw, &ch, &cd);
            if ((cx < x) && (x < (cx + cw)) &&
                (cy < y) && (y < (cy + ch)) &&
                gdk_window_is_visible (childWindow)) {
                return get_inner_gdk_window (childWindow,
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
key_event_to_context_menu_event(const nsKeyEvent* aKeyEvent,
                                nsMouseEvent* aCMEvent)
{
    memcpy(aCMEvent, aKeyEvent, sizeof(nsInputEvent));
    aCMEvent->eventStructType = NS_MOUSE_EVENT;
    aCMEvent->message = NS_CONTEXTMENU;
    aCMEvent->context = nsMouseEvent::eContextMenuKey;
    aCMEvent->button = nsMouseEvent::eRightButton;
    aCMEvent->isShift = aCMEvent->isControl = PR_FALSE;
    aCMEvent->isAlt = aCMEvent->isMeta = PR_FALSE;
    aCMEvent->clickCount = 0;
    aCMEvent->acceptActivation = PR_FALSE;
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

#ifdef ACCESSIBILITY






void
nsWindow::CreateRootAccessible()
{
    if (mIsTopLevel && !mRootAccessible) {
        nsCOMPtr<nsIAccessible> acc;
        DispatchAccessibleEvent(getter_AddRefs(acc));

        if (acc) {
            mRootAccessible = acc;
        }
    }
}

void
nsWindow::GetRootAccessible(nsIAccessible** aAccessible)
{
    nsCOMPtr<nsIAccessible> accessible, parentAccessible;
    DispatchAccessibleEvent(getter_AddRefs(accessible));
    PRUint32 role;

    if (!accessible) {
        return;
    }
    while (PR_TRUE) {
        accessible->GetParent(getter_AddRefs(parentAccessible));
        if (!parentAccessible) {
            break;
        }
        parentAccessible->GetRole(&role);
        if (role == nsIAccessibleRole::ROLE_APP_ROOT) {
            NS_ADDREF(*aAccessible = accessible);
            break;
        }
        accessible = parentAccessible;
    }
}









PRBool
nsWindow::DispatchAccessibleEvent(nsIAccessible** aAccessible)
{
    PRBool result = PR_FALSE;
    nsAccessibleEvent event(PR_TRUE, NS_GETACCESSIBLE, this);

    *aAccessible = nsnull;

    nsEventStatus status;
    DispatchEvent(&event, status);
    result = (nsEventStatus_eConsumeNoDefault == status) ? PR_TRUE : PR_FALSE;

    
    if (event.accessible)
        *aAccessible = event.accessible;

    return result;
}

void
nsWindow::DispatchActivateEvent(void)
{
    if (sAccessibilityEnabled) {
        nsCOMPtr<nsIAccessible> rootAcc;
        GetRootAccessible(getter_AddRefs(rootAcc));
        nsCOMPtr<nsPIAccessible> privAcc(do_QueryInterface(rootAcc));
        if (privAcc) {
            privAcc->FireToolkitEvent(
                         nsIAccessibleEvent::EVENT_WINDOW_ACTIVATE,
                         rootAcc, nsnull);
        }
    }

    nsCommonWidget::DispatchActivateEvent();
}

void
nsWindow::DispatchDeactivateEvent(void)
{
    nsCommonWidget::DispatchDeactivateEvent();

    if (sAccessibilityEnabled) {
        nsCOMPtr<nsIAccessible> rootAcc;
        GetRootAccessible(getter_AddRefs(rootAcc));
        nsCOMPtr<nsPIAccessible> privAcc(do_QueryInterface(rootAcc));
        if (privAcc) {
            privAcc->FireToolkitEvent(
                         nsIAccessibleEvent::EVENT_WINDOW_DEACTIVATE,
                         rootAcc, nsnull);
        }
    }
}
#endif 



nsChildWindow::nsChildWindow()
{
}

nsChildWindow::~nsChildWindow()
{
}

#ifdef USE_XIM

void
nsWindow::IMEInitData(void)
{
    if (mIMEData)
        return;
    nsWindow *win = IMEGetOwningWindow();
    if (!win)
        return;
    mIMEData = win->mIMEData;
    if (!mIMEData)
        return;
    mIMEData->mRefCount++;
}

void
nsWindow::IMEReleaseData(void)
{
    if (!mIMEData)
        return;

    mIMEData->mRefCount--;
    if (mIMEData->mRefCount != 0)
        return;

    delete mIMEData;
    mIMEData = nsnull;
}

void
nsWindow::IMEDestroyContext(void)
{
    if (!mIMEData || mIMEData->mOwner != this) {
        
        if (IMEComposingWindow() == this)
            CancelIMEComposition();
        if (gIMEFocusWindow == this)
            gIMEFocusWindow = nsnull;
        IMEReleaseData();
        return;
    }

    







    
    
    GtkIMContext *im = IMEGetContext();
    if (im && gIMEFocusWindow && gIMEFocusWindow->IMEGetContext() == im) {
        gIMEFocusWindow->IMELoseFocus();
        gIMEFocusWindow = nsnull;
    }

    mIMEData->mOwner   = nsnull;
    mIMEData->mEnabled = nsIKBStateControl::IME_STATUS_DISABLED;

    if (mIMEData->mContext) {
        gtk_im_context_set_client_window(mIMEData->mContext, nsnull);
        g_object_unref(G_OBJECT(mIMEData->mContext));
        mIMEData->mContext = nsnull;
    }

    if (mIMEData->mDummyContext) {
        gtk_im_context_set_client_window(mIMEData->mDummyContext, nsnull);
        g_object_unref(G_OBJECT(mIMEData->mDummyContext));
        mIMEData->mDummyContext = nsnull;
    }

    IMEReleaseData();
}

void
nsWindow::IMESetFocus(void)
{
    IMEInitData();

    LOGIM(("IMESetFocus %p\n", (void *)this));
    GtkIMContext *im = IMEGetContext();
    if (!im)
        return;

    gtk_im_context_focus_in(im);
    gIMEFocusWindow = this;

    if (!IMEIsEnabled()) {
        
        
        IMELoseFocus();
    }
}

void
nsWindow::IMELoseFocus(void)
{
    LOGIM(("IMELoseFocus %p\n", (void *)this));
    GtkIMContext *im = IMEGetContext();
    if (!im)
        return;

    gtk_im_context_focus_out(im);
}

void
nsWindow::IMEComposeStart(void)
{
    LOGIM(("IMEComposeStart [%p]\n", (void *)this));

    if (!mIMEData) {
        NS_ERROR("This widget doesn't support IM");
        return;
    }

    if (IMEComposingWindow()) {
        NS_WARNING("tried to re-start text composition\n");
        return;
    }

    mIMEData->mComposingWindow = this;

    nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_START, this);

    nsEventStatus status;
    DispatchEvent(&compEvent, status);

    if (NS_UNLIKELY(mIsDestroyed))
        return;

    gint x1, y1, x2, y2;
    GtkWidget *widget =
        get_gtk_widget_for_gdk_window(this->mDrawingarea->inner_window);

    gdk_window_get_origin(widget->window, &x1, &y1);
    gdk_window_get_origin(this->mDrawingarea->inner_window, &x2, &y2);

    GdkRectangle area;
    area.x = compEvent.theReply.mCursorPosition.x + (x2 - x1);
    area.y = compEvent.theReply.mCursorPosition.y + (y2 - y1);
    area.width  = 0;
    area.height = compEvent.theReply.mCursorPosition.height;

    gtk_im_context_set_cursor_location(IMEGetContext(), &area);
}

void
nsWindow::IMEComposeText(const PRUnichar *aText,
                         const PRInt32 aLen,
                         const gchar *aPreeditString,
                         const gint aCursorPos,
                         const PangoAttrList *aFeedback)
{
    if (!mIMEData) {
        NS_ERROR("This widget doesn't support IM");
        return;
    }

    
    if (!IMEComposingWindow()) {
        IMEComposeStart();
        if (NS_UNLIKELY(mIsDestroyed))
            return;
    }


    LOGIM(("IMEComposeText\n"));
    nsTextEvent textEvent(PR_TRUE, NS_TEXT_TEXT, this);

    if (aLen != 0) {
        textEvent.theText = (PRUnichar*)aText;

        if (aPreeditString && aFeedback && (aLen > 0)) {
            IM_set_text_range(aLen, aPreeditString, aCursorPos, aFeedback,
                              &(textEvent.rangeCount),
                              &(textEvent.rangeArray));
        }
    }

    nsEventStatus status;
    DispatchEvent(&textEvent, status);

    if (textEvent.rangeArray) {
        delete[] textEvent.rangeArray;
    }

    if (NS_UNLIKELY(mIsDestroyed))
        return;

    gint x1, y1, x2, y2;
    GtkWidget *widget =
        get_gtk_widget_for_gdk_window(this->mDrawingarea->inner_window);

    gdk_window_get_origin(widget->window, &x1, &y1);
    gdk_window_get_origin(this->mDrawingarea->inner_window, &x2, &y2);

    GdkRectangle area;
    area.x = textEvent.theReply.mCursorPosition.x + (x2 - x1);
    area.y = textEvent.theReply.mCursorPosition.y + (y2 - y1);
    area.width  = 0;
    area.height = textEvent.theReply.mCursorPosition.height;

    gtk_im_context_set_cursor_location(IMEGetContext(), &area);
}

void
nsWindow::IMEComposeEnd(void)
{
    LOGIM(("IMEComposeEnd [%p]\n", (void *)this));
    NS_ASSERTION(mIMEData, "This widget doesn't support IM");

    if (!IMEComposingWindow()) {
        NS_WARNING("tried to end text composition before it was started");
        return;
    }

    mIMEData->mComposingWindow = nsnull;

    nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_END, this);

    nsEventStatus status;
    DispatchEvent(&compEvent, status);
}

GtkIMContext*
nsWindow::IMEGetContext()
{
    return IM_get_input_context(this);
}

static PRBool
IsIMEEnabledState(PRUint32 aState)
{
    return aState == nsIKBStateControl::IME_STATUS_ENABLED ? PR_TRUE : PR_FALSE;
}

PRBool
nsWindow::IMEIsEnabled(void)
{
    return mIMEData ? IsIMEEnabledState(mIMEData->mEnabled) : PR_FALSE;
}

nsWindow*
nsWindow::IMEComposingWindow(void)
{
    return mIMEData ? mIMEData->mComposingWindow : nsnull;
}

nsWindow*
nsWindow::IMEGetOwningWindow(void)
{
    nsWindow *window = IM_get_owning_window(this->mDrawingarea);
    return window;
}

void
nsWindow::IMECreateContext(void)
{
    mIMEData = new nsIMEData(this);
    if (!mIMEData)
        return;

    mIMEData->mContext = gtk_im_multicontext_new();
    mIMEData->mDummyContext = gtk_im_multicontext_new();
    if (!mIMEData->mContext || !mIMEData->mDummyContext) {
        NS_ERROR("failed to create IM context.");
        IMEDestroyContext();
        return;
    }

    gtk_im_context_set_client_window(mIMEData->mContext,
                                     GTK_WIDGET(mContainer)->window);
    gtk_im_context_set_client_window(mIMEData->mDummyContext,
                                     GTK_WIDGET(mContainer)->window);

    g_signal_connect(G_OBJECT(mIMEData->mContext), "preedit_changed",
                     G_CALLBACK(IM_preedit_changed_cb), this);
    g_signal_connect(G_OBJECT(mIMEData->mContext), "commit",
                     G_CALLBACK(IM_commit_cb), this);
}

PRBool
nsWindow::IMEFilterEvent(GdkEventKey *aEvent)
{
    if (!IMEIsEnabled())
        return FALSE;

    GtkIMContext *im = IMEGetContext();
    if (!im)
        return FALSE;

    gKeyEvent = aEvent;
    gboolean filtered = gtk_im_context_filter_keypress(im, aEvent);
    gKeyEvent = NULL;

    LOGIM(("key filtered: %d committed: %d changed: %d\n",
           filtered, gKeyEventCommitted, gKeyEventChanged));

    
    
    
    
    

    PRBool retval = PR_FALSE;
    if (filtered &&
        (!gKeyEventCommitted || (gKeyEventCommitted && gKeyEventChanged)))
        retval = PR_TRUE;

    gKeyEventChanged = PR_FALSE;
    gKeyEventCommitted = PR_FALSE;
    gKeyEventChanged = PR_FALSE;

    return retval;
}


NS_IMETHODIMP
nsWindow::ResetInputState()
{
    IMEInitData();

    nsRefPtr<nsWindow> win = IMEComposingWindow();
    if (win) {
        GtkIMContext *im = IMEGetContext();
        if (!im)
            return NS_OK;

        gchar *preedit_string;
        gint cursor_pos;
        PangoAttrList *feedback_list;
        gtk_im_context_get_preedit_string(im, &preedit_string,
                                          &feedback_list, &cursor_pos);
        if (preedit_string && *preedit_string) {
            IM_commit_cb_internal(preedit_string, win);
            g_free(preedit_string);
        }
        if (feedback_list)
            pango_attr_list_unref(feedback_list);
    }

    CancelIMEComposition();

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIMEOpenState(PRBool aState)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::GetIMEOpenState(PRBool* aState)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindow::SetIMEEnabled(PRUint32 aState)
{
    IMEInitData();
    if (!mIMEData)
        return NS_OK;

    PRBool newState = IsIMEEnabledState(aState);
    PRBool oldState = IsIMEEnabledState(mIMEData->mEnabled);
    if (newState == oldState) {
        mIMEData->mEnabled = aState;
        return NS_OK;
    }

    GtkIMContext *focusedIm = nsnull;
    
    nsRefPtr<nsWindow> focusedWin = gIMEFocusWindow;
    if (focusedWin && focusedWin->mIMEData)
        focusedIm = focusedWin->mIMEData->mContext;

    if (focusedIm && focusedIm == mIMEData->mContext) {
        
        if (oldState) {
            focusedWin->ResetInputState();
            focusedWin->IMELoseFocus();
        }

        mIMEData->mEnabled = aState;

        
        
        focusedWin->IMESetFocus();
    } else {
        if (oldState)
            ResetInputState();
        mIMEData->mEnabled = aState;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsWindow::GetIMEEnabled(PRUint32* aState)
{
    NS_ENSURE_ARG_POINTER(aState);

    IMEInitData();

    *aState =
      mIMEData ? mIMEData->mEnabled : nsIKBStateControl::IME_STATUS_DISABLED;
    return NS_OK;
}

NS_IMETHODIMP
nsWindow::CancelIMEComposition()
{
    IMEInitData();

    GtkIMContext *im = IMEGetContext();
    if (!im)
        return NS_OK;

    NS_ASSERTION(!gIMESuppressCommit,
                 "CancelIMEComposition is already called!");
    gIMESuppressCommit = PR_TRUE;
    gtk_im_context_reset(im);
    gIMESuppressCommit = PR_FALSE;

    nsRefPtr<nsWindow> win = IMEComposingWindow();
    if (win) {
        win->IMEComposeText(nsnull, 0, nsnull, 0, nsnull);
        win->IMEComposeEnd();
    }

    return NS_OK;
}


void
IM_preedit_changed_cb(GtkIMContext *aContext,
                      nsWindow     *aWindow)
{
    gchar *preedit_string;
    gint cursor_pos;
    PangoAttrList *feedback_list;

    
    nsRefPtr<nsWindow> window = gFocusWindow ? gFocusWindow : gIMEFocusWindow;
    if (!window)
        return;

    
    
    gtk_im_context_get_preedit_string(aContext, &preedit_string,
                                      &feedback_list, &cursor_pos);

    LOGIM(("preedit string is: %s   length is: %d\n",
           preedit_string, strlen(preedit_string)));

    if (!preedit_string || !*preedit_string) {
        LOGIM(("preedit ended\n"));
        window->IMEComposeText(NULL, 0, NULL, 0, NULL);
        window->IMEComposeEnd();
        return;
    }

    LOGIM(("preedit len %d\n", strlen(preedit_string)));

    gunichar2 * uniStr;
    glong uniStrLen;

    uniStr = NULL;
    uniStrLen = 0;
    uniStr = g_utf8_to_utf16(preedit_string, -1, NULL, &uniStrLen, NULL);

    if (!uniStr) {
        g_free(preedit_string);
        LOG(("utf8-utf16 string tranfer failed!\n"));
        if (feedback_list)
            pango_attr_list_unref(feedback_list);
        return;
    }

    if (uniStrLen) {
        window->IMEComposeText(NS_STATIC_CAST(const PRUnichar *, uniStr),
                               uniStrLen, preedit_string, cursor_pos, feedback_list);
    }

    g_free(preedit_string);
    g_free(uniStr);

    if (feedback_list)
        pango_attr_list_unref(feedback_list);
}


void
IM_commit_cb(GtkIMContext *aContext,
             const gchar  *aUtf8_str,
             nsWindow     *aWindow)
{
    if (gIMESuppressCommit)
        return;

    LOGIM(("IM_commit_cb\n"));

    gKeyEventCommitted = PR_TRUE;

    
    nsRefPtr<nsWindow> window = gFocusWindow ? gFocusWindow : gIMEFocusWindow;

    if (!window)
        return;

    



    if (gKeyEvent) {
        char keyval_utf8[8]; 
        gint keyval_utf8_len;
        guint32 keyval_unicode;

        keyval_unicode = gdk_keyval_to_unicode(gKeyEvent->keyval);
        keyval_utf8_len = g_unichar_to_utf8(keyval_unicode, keyval_utf8);
        keyval_utf8[keyval_utf8_len] = '\0';

        if (!strcmp(aUtf8_str, keyval_utf8)) {
            gKeyEventChanged = PR_FALSE;
            return;
        }
    }

    gKeyEventChanged = PR_TRUE;
    IM_commit_cb_internal(aUtf8_str, window);
}


void
IM_commit_cb_internal(const gchar  *aUtf8_str,
                      nsWindow     *aWindow)
{
    gunichar2 *uniStr = nsnull;
    glong uniStrLen = 0;
    uniStr = g_utf8_to_utf16(aUtf8_str, -1, NULL, &uniStrLen, NULL);

    if (!uniStr) {
        LOGIM(("utf80utf16 string tranfer failed!\n"));
        return;
    }

    if (uniStrLen) {
        aWindow->IMEComposeText((const PRUnichar *)uniStr,
                                (PRInt32)uniStrLen, nsnull, 0, nsnull);
        aWindow->IMEComposeEnd();
    }

    g_free(uniStr);
}

#define	START_OFFSET(I)	\
    (*aTextRangeListResult)[I].mStartOffset

#define	END_OFFSET(I) \
    (*aTextRangeListResult)[I].mEndOffset

#define	SET_FEEDBACKTYPE(I,T) (*aTextRangeListResult)[I].mRangeType = T


void
IM_set_text_range(const PRInt32 aLen,
                  const gchar *aPreeditString,
                  const gint aCursorPos,
                  const PangoAttrList *aFeedback,
                  PRUint32 *aTextRangeListLengthResult,
                  nsTextRangeArray *aTextRangeListResult)
{
    if (aLen == 0) {
        aTextRangeListLengthResult = 0;
        aTextRangeListResult = NULL;
        return;
    }

    PangoAttrIterator * aFeedbackIterator;
    aFeedbackIterator = pango_attr_list_get_iterator((PangoAttrList*)aFeedback);
    
    
    if (aFeedbackIterator == NULL) return;

    





    PRInt32 aMaxLenOfTextRange;
    aMaxLenOfTextRange = 2*aLen + 1;
    *aTextRangeListResult = new nsTextRange[aMaxLenOfTextRange];
    NS_ASSERTION(*aTextRangeListResult, "No enough memory.");

    
    SET_FEEDBACKTYPE(0, NS_TEXTRANGE_CARETPOSITION);
    START_OFFSET(0) = aCursorPos;
    END_OFFSET(0) = aCursorPos;

    int count = 0;
    PangoAttribute * aPangoAttr;
    PangoAttribute * aPangoAttrReverse, * aPangoAttrUnderline;
    








    gint start, end;
    gunichar2 * uniStr;
    glong uniStrLen;
    do {
        aPangoAttrUnderline = pango_attr_iterator_get(aFeedbackIterator,
                                                      PANGO_ATTR_UNDERLINE);
        aPangoAttrReverse = pango_attr_iterator_get(aFeedbackIterator,
                                                    PANGO_ATTR_FOREGROUND);
        if (!aPangoAttrUnderline && !aPangoAttrReverse)
            continue;

        
        pango_attr_iterator_range(aFeedbackIterator, &start, &end);

        PRUint32 feedbackType = 0;
        
        if (aPangoAttrUnderline && aPangoAttrReverse) {
            feedbackType = NS_TEXTRANGE_SELECTEDCONVERTEDTEXT;
            
            
            
            aPangoAttr = aPangoAttrUnderline;
        }
        
        else if (aPangoAttrUnderline) {
            feedbackType = NS_TEXTRANGE_CONVERTEDTEXT;
            aPangoAttr = aPangoAttrUnderline;
        }
        
        else if (aPangoAttrReverse) {
            feedbackType = NS_TEXTRANGE_SELECTEDRAWTEXT;
            aPangoAttr = aPangoAttrReverse;
        }

        count++;
        START_OFFSET(count) = 0;
        END_OFFSET(count) = 0;

        uniStr = NULL;
        if (start > 0) {
            uniStr = g_utf8_to_utf16(aPreeditString, start,
                                     NULL, &uniStrLen, NULL);
        }
        if (uniStr) {
            START_OFFSET(count) = uniStrLen;
            g_free(uniStr);
        }

        uniStr = NULL;
        uniStr = g_utf8_to_utf16(aPreeditString + start, end - start,
                                 NULL, &uniStrLen, NULL);
        if (uniStr) {
            END_OFFSET(count) = START_OFFSET(count) + uniStrLen;
            SET_FEEDBACKTYPE(count, feedbackType);
            g_free(uniStr);
        }

    } while ((count < aMaxLenOfTextRange - 1) &&
             (pango_attr_iterator_next(aFeedbackIterator)));

   *aTextRangeListLengthResult = count + 1;

   pango_attr_iterator_destroy(aFeedbackIterator);
}


nsWindow *
IM_get_owning_window(MozDrawingarea *aArea)
{
    if (!aArea)
        return nsnull;
    GtkWidget *owningWidget =
        get_gtk_widget_for_gdk_window(aArea->inner_window);
    if (!owningWidget)
        return nsnull;
    return get_window_for_gtk_widget(owningWidget);
}


GtkIMContext *
IM_get_input_context(nsWindow *aWindow)
{
    if (!aWindow)
        return nsnull;
    nsWindow::nsIMEData *data = aWindow->mIMEData;
    if (!data)
        return nsnull;
    return data->mEnabled ? data->mContext : data->mDummyContext;
}

#endif


gfxASurface*
nsWindow::GetThebesSurface()
{
    
    
    
    
    mThebesSurface = nsnull;

    if (!mThebesSurface) {
        GdkDrawable* d = GDK_DRAWABLE(mDrawingarea->inner_window);
        gint width, height;
        gdk_drawable_get_size(d, &width, &height);
        if (!gfxPlatform::UseGlitz()) {
            mThebesSurface = new gfxXlibSurface
                (GDK_WINDOW_XDISPLAY(d),
                 GDK_WINDOW_XWINDOW(d),
                 GDK_VISUAL_XVISUAL(gdk_drawable_get_visual(d)),
                 gfxIntSize(width, height));
            gfxPlatformGtk::GetPlatform()->SetSurfaceGdkWindow(mThebesSurface, GDK_WINDOW(d));
        } else {
#ifdef MOZ_ENABLE_GLITZ
            glitz_surface_t *gsurf;
            glitz_drawable_t *gdraw;

            glitz_drawable_format_t *gdformat = glitz_glx_find_window_format (GDK_DISPLAY(),
                                                                              gdk_x11_get_default_screen(),
                                                                              0, NULL, 0);
            if (!gdformat)
                NS_ERROR("Failed to find glitz drawable format");

            Display* dpy = GDK_WINDOW_XDISPLAY(d);
            Window wnd = GDK_WINDOW_XWINDOW(d);
            
            gdraw =
                glitz_glx_create_drawable_for_window (dpy,
                                                      DefaultScreen(dpy),
                                                      gdformat,
                                                      wnd,
                                                      width,
                                                      height);
            glitz_format_t *gformat =
                glitz_find_standard_format (gdraw, GLITZ_STANDARD_RGB24);
            gsurf =
                glitz_surface_create (gdraw,
                                      gformat,
                                      width,
                                      height,
                                      0,
                                      NULL);
            glitz_surface_attach (gsurf, gdraw, GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);


            
            mThebesSurface = new gfxGlitzSurface (gdraw, gsurf, PR_TRUE);
#endif
        }
    }

    return mThebesSurface;
}
