







































#undef DEBUG_CURSORCACHE

#include <stdio.h>

#include <gtk/gtk.h>

#include <gdk/gdkx.h>
#include <gtk/gtkprivate.h>

#include <gdk/gdkprivate.h>

#include <X11/Xatom.h>   

#include "nsWindow.h"
#include "nsWidgetsCID.h"
#include "nsIFontMetrics.h"
#include "nsFont.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDOMNode.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsGfxCIID.h"
#include "nsGtkEventHandler.h"
#include "nsIAppShell.h"
#include "nsClipboard.h"
#include "nsIRollupListener.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsIProtocolHandler.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIResProtocolHandler.h"
#include "nsIFileURL.h"

#include "nsGtkUtils.h" 

#include "nsIDragService.h"
#include "nsIDragSessionGTK.h"
#include "nsAutoPtr.h"

#include "nsGtkIMEHelper.h"
#include "nsKeyboardUtils.h"

#include "nsGtkCursors.h" 

#include "nspr.h"

#include <unistd.h>

#ifdef NEED_USLEEP_PROTOTYPE
extern "C" int usleep(unsigned int);
#endif
#if defined(__QNX__)
#define usleep(s)	sleep(s)
#endif

#undef DEBUG_DND_XLATE
#undef DEBUG_DND_EVENTS
#undef DEBUG_FOCUS
#undef DEBUG_GRAB
#undef DEBUG_ICONS
#define MODAL_TIMERS_BROKEN

#define NS_TO_GDK_RGB(ns) (ns & 0xff) << 16 | (ns & 0xff00) | ((ns >> 16) & 0xff)

#define CAPS_LOCK_IS_ON \
(nsGtkUtils::gdk_keyboard_get_modifiers() & GDK_LOCK_MASK)

#define WANT_PAINT_FLASHING \
(debug_WantPaintFlashing() && CAPS_LOCK_IS_ON)

#define kWindowPositionSlop 20

static NS_DEFINE_IID(kCDragServiceCID,  NS_DRAGSERVICE_CID);

static PRBool gGlobalsInitialized   = PR_FALSE;
static PRBool gRaiseWindows         = PR_TRUE;

GdkCursor *nsWindow::gsGtkCursorCache[eCursorCount];


struct IconEntry : public PLDHashEntryHdr {
  const char* string;
  GdkPixmap* w_pixmap;
  GdkBitmap* w_mask;
  GdkPixmap* w_minipixmap;
  GdkBitmap* w_minimask;
};

static PLDHashTableOps iconHashOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashGetKeyStub,
  PL_DHashStringKey,
  PL_DHashMatchStringKey,
  PL_DHashMoveEntryStub,
  nsWindow::ClearIconEntry,
  PL_DHashFinalizeStub,
  NULL
};

gint handle_mozarea_focus_in (
    GtkWidget *      aWidget, 
    GdkEventFocus *  aGdkFocusEvent, 
    gpointer         aData);
    
gint handle_mozarea_focus_out (
    GtkWidget *      aWidget, 
    GdkEventFocus *  aGdkFocusEvent, 
    gpointer         aData);

void handle_toplevel_configure (
    GtkMozArea *      aArea,
    nsWindow   *      aWindow);


PRBool      nsWindow::sIsGrabbing = PR_FALSE;
nsWindow   *nsWindow::sGrabWindow = NULL;
PRBool      nsWindow::sIsDraggingOutOf = PR_FALSE;



GHashTable *nsWindow::mWindowLookupTable = NULL;


nsWindow *nsWindow::mLastDragMotionWindow = NULL;

PLDHashTable* nsWindow::sIconCache;

PRBool gJustGotDeactivate = PR_FALSE;
PRBool gJustGotActivate   = PR_FALSE;

#define NS_WINDOW_TITLE_MAX_LENGTH 4095

#ifdef USE_XIM

struct nsXICLookupEntry : public PLDHashEntryHdr {
  nsWindow*   mShellWindow;
  nsIMEGtkIC* mXIC;
};

PLDHashTable nsWindow::gXICLookupTable;
GdkFont *nsWindow::gPreeditFontset = nsnull;
GdkFont *nsWindow::gStatusFontset = nsnull;

#define XIC_FONTSET "-*-*-medium-r-*-*-%d-*-*-*-*-*-*-*,-*-*-*-r-*-*-%d-*-*-*-*-*-*-*,-*-*-*-*-*-*-%d-*-*-*-*-*-*-*"
#endif 

#ifdef DEBUG_DND_XLATE
static void printDepth(int depth) {
  int i;
  for (i=0; i < depth; i++)
  {
    g_print(" ");
  }
}
#endif

static int is_parent_ungrab_enter(GdkEventCrossing *aEvent);
static int is_parent_grab_leave(GdkEventCrossing *aEvent);

NS_IMETHODIMP nsWindow::SetParent(nsIWidget* aNewParent)
{
  NS_ENSURE_ARG_POINTER(aNewParent);

  GdkWindow* newParentWindow =
    NS_STATIC_CAST(GdkWindow*, aNewParent->GetNativeData(NS_NATIVE_WINDOW));
  NS_ASSERTION(newParentWindow, "Parent widget has a null native window handle");

  if (!mShell && mSuperWin) {
    gdk_superwin_reparent(mSuperWin, newParentWindow);
  } else {
    NS_NOTREACHED("nsWindow::SetParent - reparenting a non-child window");
  }
  return NS_OK;
}




static PRBool
ButtonEventInsideWindow (GdkWindow *window, GdkEventButton *aGdkButtonEvent)
{
  gint x, y;
  gint width, height;
  gdk_window_get_position(window, &x, &y);
  gdk_window_get_size(window, &width, &height);

  
  
  if (aGdkButtonEvent->x >= x && aGdkButtonEvent->y >= y &&
      aGdkButtonEvent->x <= width + x && aGdkButtonEvent->y <= height + y)
    return TRUE;

  return FALSE;
}

NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsWidget)






nsWindow::nsWindow() 
{
  mShell = nsnull;
  mWindowType = eWindowType_child;
  mBorderStyle = eBorderStyle_default;
  mSuperWin = 0;
  mMozArea = 0;
  mMozAreaClosestParent = 0;
  mCachedX = mCachedY = -1;

  mIsTooSmall = PR_FALSE;
  mIsUpdating = PR_FALSE;
  mTransientParent = nsnull;
  
  if (mWindowLookupTable == NULL) {
    mWindowLookupTable = g_hash_table_new(g_direct_hash, g_direct_equal);
  }
  if (mLastDragMotionWindow == this)
    mLastDragMotionWindow = NULL;
  mBlockMozAreaFocusIn = PR_FALSE;
  mLastGrabFailed = PR_TRUE;
  mHasAnonymousChildren = PR_FALSE;
  mDragMotionWidget = 0;
  mDragMotionContext = 0;
  mDragMotionX = 0;
  mDragMotionY = 0;
  mDragMotionTime = 0;
  mDragMotionTimerID = 0;

  
  mIMECompositionUniString = nsnull;
  mIMECompositionUniStringSize = 0;

  mIsTranslucent = PR_FALSE;
  mTransparencyBitmap = nsnull;

#ifdef USE_XIM
  mIMEEnable = PR_TRUE; 
  mIMEShellWindow = 0;
  mIMECallComposeStart = PR_FALSE;
  mIMECallComposeEnd = PR_TRUE;
  mIMEIsBeingActivate = PR_FALSE;
  mICSpotTimer = nsnull;
  mXICFontSize = 16;
  if (gXICLookupTable.ops == NULL) {
    PL_DHashTableInit(&gXICLookupTable, PL_DHashGetStubOps(), nsnull,
                      sizeof(nsXICLookupEntry), PL_DHASH_MIN_SIZE);
  }
#endif 

  mLeavePending = PR_FALSE;
  mRestoreFocus = PR_FALSE;

  
  if (!gGlobalsInitialized) {
    gGlobalsInitialized = PR_TRUE;

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
      PRBool val = PR_TRUE;
      nsresult rv;
      rv = prefs->GetBoolPref("mozilla.widget.raise-on-setfocus",
                              &val);
      if (NS_SUCCEEDED(rv))
        gRaiseWindows = val;

      
      
      
      PRBool grab_during_popup = PR_TRUE;
      PRBool ungrab_during_mode_switch = PR_TRUE;
      prefs->GetBoolPref("autocomplete.grab_during_popup",
                              &grab_during_popup);
      prefs->GetBoolPref("autocomplete.ungrab_during_mode_switch",
                              &ungrab_during_mode_switch);
      nsXKBModeSwitch::ControlWorkaround(grab_during_popup,
                           ungrab_during_mode_switch);
    }

    sIconCache = PL_NewDHashTable(&iconHashOps, nsnull, sizeof(IconEntry), 28);
  }
}






nsWindow::~nsWindow()
{
#ifdef USE_XIM
  KillICSpotTimer();
#endif 

  if (mIMECompositionUniString) {
    delete[] mIMECompositionUniString;
    mIMECompositionUniString = nsnull;
  }

  
  ResetDragMotionTimer(0, 0, 0, 0, 0);

  
  
  if (sGrabWindow == this) {
    sIsGrabbing = PR_FALSE;
    sGrabWindow = NULL;
  }
  
  
  if (mLastDragMotionWindow == this) {
    mLastDragMotionWindow = NULL;
  }
  
  if (mHasFocus == PR_TRUE) {
    sFocusWindow = NULL;
  }

  
  

  Destroy();

  delete[] mTransparencyBitmap;
  mTransparencyBitmap = nsnull;

  if (mIsUpdating)
    UnqueueDraw();
}

 void
nsWindow::ReleaseGlobals()
{
  if (mWindowLookupTable) {
    g_hash_table_destroy(mWindowLookupTable);
    mWindowLookupTable = nsnull;
  }
  if (gXICLookupTable.ops) {
    PL_DHashTableFinish(&gXICLookupTable);
    gXICLookupTable.ops = nsnull;
  }
  if (sIconCache) {
    PL_DHashTableDestroy(sIconCache);
    sIconCache = nsnull;
  }
  if (gPreeditFontset) {
    gdk_font_unref(gPreeditFontset);
    gPreeditFontset = nsnull;
  }
  if (gStatusFontset) {
    gdk_font_unref(gStatusFontset);
    gStatusFontset = nsnull;
  }
  for (int i = 0, n = NS_ARRAY_LENGTH(gsGtkCursorCache); i < n; ++i) {
    if (gsGtkCursorCache[i]) {
      gdk_cursor_destroy(gsGtkCursorCache[i]);
      gsGtkCursorCache[i] = nsnull;
    }
  }
  gGlobalsInitialized = PR_FALSE;
}

NS_IMETHODIMP nsWindow::Destroy(void)
{
  
  

  if (mSuperWin)
    gtk_object_remove_data(GTK_OBJECT(mSuperWin), "nsWindow");
  if (mShell)
    gtk_object_remove_data(GTK_OBJECT(mShell), "nsWindow");
  if (mMozArea)
    gtk_object_remove_data(GTK_OBJECT(mMozArea), "nsWindow");

  return nsWidget::Destroy();
}


void nsWindow::InvalidateWindowPos(void)
{
  mCachedX = mCachedY = -1;

  for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
    
    nsRect kidBounds;
    kid->GetBounds(kidBounds);
    kid->Move(kidBounds.x, kidBounds.y);
  }

  
  
  if (mSuperWin && mHasAnonymousChildren) {
    Display *display = GDK_DISPLAY();
    Window  window = GDK_WINDOW_XWINDOW(mSuperWin->bin_window);
    if (window && !((GdkWindowPrivate *)mSuperWin->bin_window)->destroyed)
    {
      Window       root_return;
      Window       parent_return;
      Window      *children_return = NULL;
      unsigned int nchildren_return = 0;
      
      XQueryTree(display, window, &root_return, &parent_return,
                 &children_return, &nchildren_return);
      
      for (unsigned int i=0; i < nchildren_return; i++)
      {
        Window child_window = children_return[i];
        nsWindow *thisWindow = GetnsWindowFromXWindow(child_window);
        if (thisWindow)
        {
          nsRect kidBounds;
          thisWindow->GetBounds(kidBounds);
          thisWindow->Move(kidBounds.x, kidBounds.y);
        }
      }      
    }
  }
}

void
handle_invalidate_pos(GtkMozArea *aArea, gpointer p)
{
  nsWindow *widget = (nsWindow *)p;
  widget->InvalidateWindowPos();
}

PRBool nsWindow::GetWindowPos(nscoord &x, nscoord &y)
{
  if ((mCachedX==-1) && (mCachedY==-1)) { 
    gint xpos, ypos;

    if (mMozArea)
      {
        if (mMozArea->window)
          {
            if (!GTK_WIDGET_MAPPED(mMozArea) || !GTK_WIDGET_REALIZED(mMozArea)) {
              
              return PR_FALSE;
            }
            gdk_window_get_root_origin(mMozArea->window, &xpos, &ypos);
          }
        else
          return PR_FALSE;
      }
    else if (mSuperWin)
      {
        if (mSuperWin->bin_window)
          {
            gdk_window_get_origin(mSuperWin->bin_window, &xpos, &ypos);
          }
        else
          return PR_FALSE;
      }
    else
      return PR_FALSE;

    mCachedX = xpos;
    mCachedY = ypos;
  }

  x = mCachedX;
  y = mCachedY;

  return PR_TRUE;
}

NS_IMETHODIMP nsWindow::WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect)
{
  nscoord x;
  nscoord y;

  aNewRect.width = aOldRect.width;
  aNewRect.height = aOldRect.height;

  if (!GetWindowPos(x, y))
    return NS_ERROR_FAILURE;
 
  aNewRect.x = x + aOldRect.x;
  aNewRect.y = y + aOldRect.y;

  return NS_OK;
}


 

void
nsWindow::DestroyNative(void)
{
  
  
  DestroyNativeChildren();

#ifdef USE_XIM
  IMEDestroyIC();
#endif 

  if (mSuperWin) {
    
    g_hash_table_remove(mWindowLookupTable, mSuperWin->shell_window);
  }

  if (mShell) {
    gtk_widget_destroy(mShell);
    mShell = nsnull;
    
    mMozArea = nsnull;
    mSuperWin = nsnull;
  }
  else if(mMozArea) {
    
    
    gtk_widget_destroy(mMozArea);
    mMozArea = nsnull;
    mSuperWin = nsnull;
  }
  else if(mSuperWin) {
    gtk_object_unref(GTK_OBJECT(mSuperWin));
    mSuperWin = NULL;
  }
}







void
nsWindow::DestroyNativeChildren(void)
{

  Display     *display;
  Window       window;
  Window       root_return;
  Window       parent_return;
  Window      *children_return = NULL;
  unsigned int nchildren_return = 0;
  unsigned int i = 0;

  if (mSuperWin)
  {
    display = GDK_DISPLAY();
    window = GDK_WINDOW_XWINDOW(mSuperWin->bin_window);
    if (window && !((GdkWindowPrivate *)mSuperWin->bin_window)->destroyed)
    {
      
      XQueryTree(display, window, &root_return, &parent_return,
                 &children_return, &nchildren_return);
      
      for (i=0; i < nchildren_return; i++)
      {
        Window child_window = children_return[i];
        nsWindow *thisWindow = GetnsWindowFromXWindow(child_window);
        if (thisWindow)
        {
          thisWindow->Destroy();
        }
      }      
    }
  }

  
  if (children_return)
    XFree(children_return);
}





nsWindow *
nsWindow::GetnsWindowFromXWindow(Window aWindow)
{
  GdkWindow *thisWindow = NULL;

  thisWindow = gdk_window_lookup(aWindow);

  if (!thisWindow)
  {
    return NULL;
  }
  gpointer data = NULL;
  
  gdk_window_get_user_data(thisWindow, &data);
  if (data)
  {
    if (GTK_IS_OBJECT(data))
    {
      return (nsWindow *)gtk_object_get_data(GTK_OBJECT(data), "nsWindow");
    }
    else
    {
      return NULL;
    }
  }
  else
  {
    
    nsWindow *childWindow = (nsWindow *)g_hash_table_lookup(nsWindow::mWindowLookupTable,
                                                            thisWindow);
    if (childWindow)
    {
      return childWindow;
    }
  }
  
  return NULL;
}








Window
nsWindow::GetInnerMostWindow(Window aOriginWindow,
                             Window aWindow,
                             nscoord x, nscoord y,
                             nscoord *retx, nscoord *rety,
                             int depth)
{

  Display     *display;
  Window       window;
  Window       root_return;
  Window       parent_return;
  Window      *children_return = NULL;
  unsigned int nchildren_return;
  unsigned int i;
  Window       returnWindow = None;
  
  display = GDK_DISPLAY();
  window = aWindow;

#ifdef DEBUG_DND_XLATE
  printDepth(depth);
  g_print("Finding children for 0x%lx\n", aWindow);
#endif

  
  XQueryTree(display, window, &root_return, &parent_return,
             &children_return, &nchildren_return);
  
#ifdef DEBUG_DND_XLATE
  printDepth(depth);
  g_print("Found %d children\n", nchildren_return);
#endif

  

  for (i=0; i < nchildren_return; i++)
  {
    Window src_w = aOriginWindow;
    Window dest_w = children_return[i];
    int  src_x = x;
    int  src_y = y;
    int  dest_x_return, dest_y_return;
    Window child_return;
    
#ifdef DEBUG_DND_XLATE
    printDepth(depth);
    g_print("Checking window 0x%lx with coords %d %d\n", dest_w,
            src_x, src_y);
#endif
    if (XTranslateCoordinates(display, src_w, dest_w,
                              src_x, src_y,
                              &dest_x_return, &dest_y_return,
                              &child_return))
    {
      int x_return, y_return;
      unsigned int width_return, height_return;
      unsigned int border_width_return;
      unsigned int depth_return;

      
      XGetGeometry(display, src_w, &root_return, &x_return, &y_return,
                   &width_return, &height_return, &border_width_return,
                   &depth_return);

#ifdef DEBUG_DND_XLATE
      printDepth(depth);
      g_print("parent has geo %d %d %d %d\n",
              x_return, y_return, width_return, height_return);
#endif

      
      XGetGeometry(display, dest_w, &root_return, &x_return, &y_return,
                   &width_return, &height_return, &border_width_return,
                   &depth_return);

#ifdef DEBUG_DND_XLATE
      printDepth(depth);
      g_print("child has geo %d %d %d %d\n",
              x_return, y_return, width_return, height_return);
      printDepth(depth);
      g_print("coords are %d %d in child window's geo\n",
              dest_x_return, dest_y_return);
#endif

      int x_offset = width_return;
      int y_offset = height_return;
      x_offset -= dest_x_return;
      y_offset -= dest_y_return;
#ifdef DEBUG_DND_XLATE
      printDepth(depth);
      g_print("offsets are %d %d\n", x_offset, y_offset);
#endif
      
      if ((dest_x_return > 0) && (dest_y_return > 0) &&
          (y_offset > 0) && (x_offset > 0))
      {
        
        returnWindow = dest_w;
        
        *retx = dest_x_return;
        *rety = dest_y_return;
        
        
        Window tempWindow = None;
        tempWindow = GetInnerMostWindow(aOriginWindow, dest_w, x, y, retx, rety, (depth + 1));
        if (tempWindow != None)
          returnWindow = tempWindow;
        goto finishedWalk;
      }
      
    }
    else
    {
      g_assert("XTranslateCoordinates failed!\n");
    }
  }
  
 finishedWalk:

  
  if (children_return)
    XFree(children_return);

  return returnWindow;
}








static GSList *update_queue = NULL;
static guint update_idle = 0;

gboolean 
nsWindow::UpdateIdle (gpointer data)
{
  GSList *old_queue = update_queue;
  GSList *it;
  
  update_idle = 0;
  update_queue = nsnull;
  
  for (it = old_queue; it; it = it->next)
  {
    nsWindow *window = (nsWindow *)it->data;
    window->mIsUpdating = PR_FALSE;
  }
  
  for (it = old_queue; it; it = it->next)
  {
    nsWindow *window = (nsWindow *)it->data;
    window->Update();
  }
  
  g_slist_free (old_queue);
  
  return PR_FALSE;
}

void
nsWindow::QueueDraw ()
{
  if (!mIsUpdating)
  {
    update_queue = g_slist_prepend (update_queue, (gpointer)this);
    if (!update_idle)
      update_idle = g_idle_add_full (G_PRIORITY_HIGH_IDLE, 
                                     UpdateIdle,
                                     NULL, (GDestroyNotify)NULL);
    mIsUpdating = PR_TRUE;
  }
}

void
nsWindow::UnqueueDraw ()
{
  if (mIsUpdating)
  {
    update_queue = g_slist_remove (update_queue, (gpointer)this);
    mIsUpdating = PR_FALSE;
  }
}

void 
nsWindow::DoPaint (nsIRegion *aClipRegion)
{
  if (!mEventCallback)
    return;

  
  if (!mSuperWin)
    return;

  if (mSuperWin->visibility == GDK_VISIBILITY_FULLY_OBSCURED) {
    
    
    
    
    PRBool isTranslucent;
    GetWindowTranslucency(isTranslucent);
    if (!isTranslucent)
      return;
  }

  nsCOMPtr<nsIRenderingContext> rc = getter_AddRefs(GetRenderingContext());
  if (!rc)
    return;




#undef  NS_PAINT_SEPARATELY

#ifdef NS_PAINT_SEPARATELY
  nsRegionRectSet *regionRectSet = nsnull;
  aClipRegion->GetRects(&regionRectSet);
  for (nsRegionRect *r = regionRectSet->mRects,
                *r_end = r + regionRectSet->mNumRects; r < r_end; ++r) {
  nsRect boundsRect(r->x, r->y, r->width, r->height);
#else
  nsRect boundsRect;
  aClipRegion->GetBoundingBox(&boundsRect.x, &boundsRect.y, &boundsRect.width, &boundsRect.height);
#endif

  nsPaintEvent event(PR_TRUE, NS_PAINT, this);
  event.renderingContext = rc;
  event.time = GDK_CURRENT_TIME; 
  event.rect = &boundsRect;
  
#ifdef DEBUG
  GdkWindow *gw = GetRenderWindow(GTK_OBJECT(mSuperWin));
  if (WANT_PAINT_FLASHING && gw)
    {
      GdkRegion *region;
      aClipRegion->GetNativeRegion(*(void**)&region);
      nsGtkUtils::gdk_window_flash(gw,1,100000,region);
    }

  
  
  if (debug_GetCachedBoolPref("nglayout.debug.paint_dumping") && CAPS_LOCK_IS_ON)
    debug_DumpPaintEvent(stdout, this, &event, 
                         debug_GetName(GTK_OBJECT(mSuperWin)),
                         (PRInt32) debug_GetRenderXID(GTK_OBJECT(mSuperWin)));
#endif 

  DispatchWindowEvent(&event);
#ifdef NS_PAINT_SEPARATELY
  }
#endif
}

static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

NS_IMETHODIMP nsWindow::Update(void)
{
  if (!mSuperWin)               
    return NS_OK;

  if (mIsUpdating)
    UnqueueDraw();

  if (!mUpdateArea->IsEmpty()) {
    
    
    nsCOMPtr<nsIRegion> updateArea = mUpdateArea;
    mUpdateArea = do_CreateInstance(kRegionCID);
    if (mUpdateArea) {
      mUpdateArea->Init();
      mUpdateArea->SetTo(0, 0, 0, 0);
    }
    
    DoPaint(updateArea);
  } else {
    
  }

  
  
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener,
                                            PRBool aDoCapture,
                                            PRBool aConsumeRollupEvent)
{
  GtkWidget *grabWidget;

  grabWidget = mWidget;
  

  if (aDoCapture) {

    if (mSuperWin) {

      
      if (!nsWindow::DragInProgress()) {
        NativeGrab(PR_TRUE);

        sIsGrabbing = PR_TRUE;
        sGrabWindow = this;
      }
    }

    gRollupListener = aListener;
    gRollupWidget = do_GetWeakReference(NS_STATIC_CAST(nsIWidget*,this));
  } else {
    
    if (sGrabWindow == this) {
      sGrabWindow = NULL;
    }
    sIsGrabbing = PR_FALSE;

    if (!nsWindow::DragInProgress())
      NativeGrab(PR_FALSE);
    gRollupListener = nsnull;
    gRollupWidget = nsnull;
  }
  
  return NS_OK;
}





void nsWindow::NativeGrab(PRBool aGrab)
{
  
  mLastGrabFailed = PR_FALSE;

  if (aGrab) {
    DropMotionTarget();
    gint retval;
    retval = gdk_pointer_grab (GDK_SUPERWIN(mSuperWin)->bin_window, PR_TRUE, (GdkEventMask)
                               (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                                GDK_POINTER_MOTION_MASK),
                               (GdkWindow*)NULL, NULL, GDK_CURRENT_TIME);
#ifdef DEBUG_GRAB
    printf("nsWindow::NativeGrab %p pointer_grab %d\n", this, retval);
#endif
    
    if (retval != 0)
      mLastGrabFailed = PR_TRUE;

    if (mTransientParent)
      retval = nsXKBModeSwitch::GrabKeyboard(GTK_WIDGET(mTransientParent)->window,
                                 PR_TRUE, GDK_CURRENT_TIME);
    else
      retval = nsXKBModeSwitch::GrabKeyboard(mSuperWin->bin_window,
                                 PR_TRUE, GDK_CURRENT_TIME);
#ifdef DEBUG_GRAB
    printf("nsWindow::NativeGrab %p keyboard_grab %d\n", this, retval);
#endif
    
    if (retval != 0)
      mLastGrabFailed = PR_TRUE;

    
    gtk_grab_add(GetOwningWidget());
  } else {
#ifdef DEBUG_GRAB
    printf("nsWindow::NativeGrab %p ungrab\n", this);
#endif
    nsXKBModeSwitch::UnGrabKeyboard(GDK_CURRENT_TIME);
    gtk_grab_remove(GetOwningWidget());
    DropMotionTarget();
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    
    
    gdk_flush();
  }
}

NS_IMETHODIMP nsWindow::Validate()
{
  if (mIsUpdating) {
    mUpdateArea->SetTo(0, 0, 0, 0);
    UnqueueDraw();
  }
  return NS_OK;
}

NS_IMETHODIMP nsWindow::Invalidate(PRBool aIsSynchronous)
{

  if (!mSuperWin)
    return NS_OK;
  
  mUpdateArea->SetTo(0, 0, mBounds.width, mBounds.height);
  
  if (aIsSynchronous)
    Update();
  else
    QueueDraw();
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::Invalidate(const nsRect &aRect, PRBool aIsSynchronous)
{

  if (!mSuperWin)
    return NS_OK;

  mUpdateArea->Union(aRect.x, aRect.y, aRect.width, aRect.height);

  if (aIsSynchronous)
    Update();
  else
    QueueDraw();
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous)
{

  if (!mSuperWin)
    return NS_OK;
  
  mUpdateArea->Union(*aRegion);

  if (aIsSynchronous)
    Update();
  else
    QueueDraw();
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetBackgroundColor(const nscolor &aColor)
{
  nsBaseWidget::SetBackgroundColor(aColor);

  if (nsnull != mSuperWin) {
    GdkColor back_color;

    back_color.pixel = ::gdk_rgb_xpixel_from_rgb(NS_TO_GDK_RGB(aColor));

    gdk_window_set_background(mSuperWin->bin_window, &back_color); 
  }

  return NS_OK;
}






NS_IMETHODIMP nsWindow::SetCursor(nsCursor aCursor)
{
  if (!mSuperWin) 
    


    return NS_ERROR_FAILURE;

  
  
  if (!mMozArea)
    return GetOwningWindow()->SetCursor(aCursor);

  
  if (aCursor != mCursor) {
    GdkCursor *newCursor = 0;

    newCursor = GtkCreateCursor(aCursor);

    if (nsnull != newCursor) {
      mCursor = aCursor;
      ::gdk_window_set_cursor(mSuperWin->shell_window, newCursor);
      XFlush(GDK_DISPLAY());
    }
  }
  return NS_OK;
}

GdkCursor *nsWindow::GtkCreateCursor(nsCursor aCursorType)
{
  GdkPixmap *cursor;
  GdkPixmap *mask;
  GdkColor fg, bg;
  GdkCursor *gdkcursor = nsnull;
  PRUint8 newType = 0xff;

  if ((gdkcursor = gsGtkCursorCache[aCursorType])) {
#ifdef DEBUG_CURSORCACHE
    printf("cached cursor found: %p\n", gdkcursor);
#endif
    return gdkcursor;
  }

  switch (aCursorType) {
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
      NS_ASSERTION(aCursorType, "Invalid cursor type");
      gdkcursor = gdk_cursor_new(GDK_LEFT_PTR);
      break;
  }

  
  if (newType != 0xff) {
    gdk_color_parse("#000000", &fg);
    gdk_color_parse("#ffffff", &bg);

    cursor = gdk_bitmap_create_from_data(NULL,
                                         (char *)GtkCursors[newType].bits,
                                         32, 32);
    mask   = gdk_bitmap_create_from_data(NULL,
                                         (char *)GtkCursors[newType].mask_bits,
                                         32, 32);

    gdkcursor = gdk_cursor_new_from_pixmap(cursor, mask, &fg, &bg,
                                           GtkCursors[newType].hot_x,
                                           GtkCursors[newType].hot_y);

    gdk_bitmap_unref(mask);
    gdk_bitmap_unref(cursor);
  }

#ifdef DEBUG_CURSORCACHE
  printf("inserting cursor into the cache: %p\n", gdkcursor);
#endif
  gsGtkCursorCache[aCursorType] = gdkcursor;

  return gdkcursor;
}

NS_IMETHODIMP
nsWindow::Enable(PRBool aState)
{
  GtkWidget *top_mozarea = GetOwningWidget();
  GtkWindow *top_window = GTK_WINDOW(gtk_widget_get_toplevel(top_mozarea));

  if (aState) {
    gtk_widget_set_sensitive(top_mozarea, TRUE);
    
    
    
    
    
    if (mRestoreFocus && !top_window->focus_widget) {
      gtk_window_set_focus(top_window, top_mozarea);
    }
    mRestoreFocus = PR_FALSE;
  }
  else {
    
    
    
    
    if (top_window->focus_widget == top_mozarea) {
      mRestoreFocus = PR_TRUE;
    }
    gtk_widget_set_sensitive(top_mozarea, FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::IsEnabled(PRBool *aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  *aState = !mMozArea || GTK_WIDGET_IS_SENSITIVE(mMozArea);

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetFocus(PRBool aRaise)
{
#ifdef DEBUG_FOCUS
  printf("nsWindow::SetFocus %p\n", NS_STATIC_CAST(void *, this));
#endif 

  GtkWidget *top_mozarea = GetOwningWidget();
  GtkWidget *toplevel = nsnull;

  if (top_mozarea)
    toplevel = gtk_widget_get_toplevel(top_mozarea);

  
  
  if (gRaiseWindows && aRaise && toplevel && top_mozarea &&
      (!GTK_WIDGET_HAS_FOCUS(top_mozarea) && !GTK_WIDGET_HAS_FOCUS(toplevel)))
    GetAttention(-1);

#ifdef DEBUG_FOCUS
  printf("top moz area is %p\n", NS_STATIC_CAST(void *, top_mozarea));
#endif

  
  gboolean toplevel_focus =
    gtk_mozarea_get_toplevel_focus(GTK_MOZAREA(top_mozarea));

  
  
  if (top_mozarea && !GTK_WIDGET_HAS_FOCUS(top_mozarea)) {

    gpointer data = gtk_object_get_data(GTK_OBJECT(top_mozarea), "nsWindow");
    nsWindow *mozAreaWindow = NS_STATIC_CAST(nsWindow *, data);
    mozAreaWindow->mBlockMozAreaFocusIn = PR_TRUE;
    gtk_widget_grab_focus(top_mozarea);
    mozAreaWindow->mBlockMozAreaFocusIn = PR_FALSE;

    
    
    
    
    if (!toplevel_focus)
      GTK_WIDGET_UNSET_FLAGS(top_mozarea, GTK_HAS_FOCUS);
    
    
    DispatchSetFocusEvent();
    return NS_OK;
  }

  if (mHasFocus)
  {
#ifdef DEBUG_FOCUS
    printf("Returning: Already have focus.\n");
#endif 
    return NS_OK;
  }

  
  if (sFocusWindow)
  {
    
    sFocusWindow->DispatchLostFocusEvent();
    sFocusWindow->LoseFocus();
  }

  

  sFocusWindow = this;
  mHasFocus = PR_TRUE;

#ifdef USE_XIM
  IMESetFocusWindow();
#endif 

  DispatchSetFocusEvent();

#ifdef DEBUG_FOCUS
  printf("Returning:\n");
#endif

  return NS_OK;
}

 void
nsWindow::LoseFocus(void)
{
  
  if (mHasFocus == PR_FALSE)
    return;

#ifdef USE_XIM
  IMEUnsetFocusWindow();
#endif 
  
  sFocusWindow = 0;
  mHasFocus = PR_FALSE;

}

void nsWindow::DispatchSetFocusEvent(void)
{
#ifdef DEBUG_FOCUS
  printf("nsWindow::DispatchSetFocusEvent %p\n", NS_STATIC_CAST(void *, this));
#endif 

  nsGUIEvent event(PR_TRUE, NS_GOTFOCUS, this);

  NS_ADDREF_THIS();
  DispatchFocus(event);

  if (gJustGotActivate) {
    gJustGotActivate = PR_FALSE;
    DispatchActivateEvent();
  }

  NS_RELEASE_THIS();
}

void nsWindow::DispatchLostFocusEvent(void)
{

#ifdef DEBUG_FOCUS
  printf("nsWindow::DispatchLostFocusEvent %p\n", NS_STATIC_CAST(void *, this));
#endif 

  nsGUIEvent event(PR_TRUE, NS_LOSTFOCUS, this);

  NS_ADDREF_THIS();
  
  DispatchFocus(event);
  
  NS_RELEASE_THIS();
}

void nsWindow::DispatchActivateEvent(void)
{
#ifdef DEBUG_FOCUS
  printf("nsWindow::DispatchActivateEvent %p\n", NS_STATIC_CAST(void *, this));
#endif

#ifdef USE_XIM
  IMEBeingActivate(PR_TRUE);
#endif 

  gJustGotDeactivate = PR_FALSE;

  nsGUIEvent event(PR_TRUE, NS_ACTIVATE, this);

  NS_ADDREF_THIS();  
  DispatchFocus(event);
  NS_RELEASE_THIS();

#ifdef USE_XIM
  IMEBeingActivate(PR_FALSE);
#endif 
}

void nsWindow::DispatchDeactivateEvent(void)
{
#ifdef DEBUG_FOCUS
  printf("nsWindow::DispatchDeactivateEvent %p\n", 
         NS_STATIC_CAST(void *, this));
#endif
#ifdef USE_XIM
  IMEBeingActivate(PR_TRUE);
#endif 

  nsGUIEvent event(PR_TRUE, NS_DEACTIVATE, this);

  NS_ADDREF_THIS();
  DispatchFocus(event);
  NS_RELEASE_THIS();

#ifdef USE_XIM
  IMEBeingActivate(PR_FALSE);
#endif 
}




void nsWindow::HandleMozAreaFocusIn(void)
{
  
  
  
  if (mBlockMozAreaFocusIn)
    return;

  
#ifdef DEBUG_FOCUS
  printf("nsWindow::HandleMozAreaFocusIn %p\n", NS_STATIC_CAST(void *, this));
#endif
  
  
  if (mIsToplevel)
    gJustGotActivate = PR_TRUE;

#ifdef USE_XIM
  IMESetFocusWindow();
#endif 

  DispatchSetFocusEvent();
}




void nsWindow::HandleMozAreaFocusOut(void)
{
  
#ifdef DEBUG_FOCUS
  printf("nsWindow::HandleMozAreaFocusOut %p\n", NS_STATIC_CAST(void *, this));
#endif
  
  
  if (sFocusWindow)
  {
    
    
    
    PRBool isChild = PR_FALSE;
    GdkWindow *window;
    window = (GdkWindow *)sFocusWindow->GetNativeData(NS_NATIVE_WINDOW);
    while (window)
    {
      gpointer data = NULL;
      gdk_window_get_user_data(window, &data);
      if (GTK_IS_MOZAREA(data)) 
      {
        GtkWidget *tmpMozArea = GTK_WIDGET(data);
        if (tmpMozArea == mMozArea)
        {
          isChild = PR_TRUE;
          break;
        }
      }
      window = gdk_window_get_parent(window);
    }

    if (isChild)
    {
      nsWidget *focusWidget = sFocusWindow;
      nsCOMPtr<nsIWidget> focusWidgetGuard(focusWidget);

      focusWidget->DispatchLostFocusEvent();
      
      
      if (mIsToplevel)
        focusWidget->DispatchDeactivateEvent();
      focusWidget->LoseFocus();
    }
  }
}


 void
nsWindow::OnMotionNotifySignal(GdkEventMotion *aGdkMotionEvent)
{
  XEvent xevent;
  GdkEvent gdk_event;
  PRBool synthEvent = PR_FALSE;
  while (XCheckWindowEvent(GDK_DISPLAY(),
                           GDK_WINDOW_XWINDOW(mSuperWin->bin_window),
                           ButtonMotionMask, &xevent)) {
    synthEvent = PR_TRUE;
  }
  if (synthEvent) {
    gdk_event.type = GDK_MOTION_NOTIFY;
    gdk_event.motion.window = aGdkMotionEvent->window;
    gdk_event.motion.send_event = aGdkMotionEvent->send_event;
    gdk_event.motion.time = xevent.xmotion.time;
    gdk_event.motion.x = xevent.xmotion.x;
    gdk_event.motion.y = xevent.xmotion.y;
    gdk_event.motion.pressure = aGdkMotionEvent->pressure;
    gdk_event.motion.xtilt = aGdkMotionEvent->xtilt;
    gdk_event.motion.ytilt = aGdkMotionEvent->ytilt;
    gdk_event.motion.state = aGdkMotionEvent->state;
    gdk_event.motion.is_hint = xevent.xmotion.is_hint;
    gdk_event.motion.source = aGdkMotionEvent->source;
    gdk_event.motion.deviceid = aGdkMotionEvent->deviceid;
    gdk_event.motion.x_root = xevent.xmotion.x_root;
    gdk_event.motion.y_root = xevent.xmotion.y_root;
    nsWidget::OnMotionNotifySignal(&gdk_event.motion);
  }
  else {
    nsWidget::OnMotionNotifySignal(aGdkMotionEvent);
  }
}


 void
nsWindow::OnEnterNotifySignal(GdkEventCrossing *aGdkCrossingEvent)
{
  if (GTK_WIDGET_SENSITIVE(GetOwningWidget())) {
    nsWidget::OnEnterNotifySignal(aGdkCrossingEvent);
    if (mMozArea) {
      GTK_PRIVATE_SET_FLAG(mMozArea, GTK_LEAVE_PENDING);
      mLeavePending = PR_TRUE;
    }
  }
}


 void
nsWindow::OnLeaveNotifySignal(GdkEventCrossing *aGdkCrossingEvent)
{
  if (mMozArea) {
    if (mLeavePending) {
      GTK_PRIVATE_UNSET_FLAG(mMozArea, GTK_LEAVE_PENDING);
      mLeavePending = PR_FALSE;
      nsWidget::OnLeaveNotifySignal(aGdkCrossingEvent);
    }
  } else
    nsWidget::OnLeaveNotifySignal(aGdkCrossingEvent);
}


 void
nsWindow::OnButtonPressSignal(GdkEventButton *aGdkButtonEvent)
{
  
  
  
  
  
  if (gRollupWidget && ((GetOwningWindowType() != eWindowType_popup) ||
                        (mSuperWin->bin_window == aGdkButtonEvent->window &&
                         !ButtonEventInsideWindow(aGdkButtonEvent->window,
                                                  aGdkButtonEvent)))) {
    gRollupListener->Rollup();
    gRollupWidget = nsnull;
    gRollupListener = nsnull;
    return;
  }

  nsWidget::OnButtonPressSignal(aGdkButtonEvent);
}


 void
nsWindow::OnButtonReleaseSignal(GdkEventButton *aGdkButtonEvent)
{
  
  
  
  if (!sButtonMotionTarget &&
      (gRollupWidget && GetOwningWindowType() != eWindowType_popup)) {
    return;
  }
  nsWidget::OnButtonReleaseSignal(aGdkButtonEvent);
}


 void
nsWindow::OnFocusInSignal(GdkEventFocus * aGdkFocusEvent)
{
  
  GTK_WIDGET_SET_FLAGS(mMozArea, GTK_HAS_FOCUS);

  nsFocusEvent event(PR_TRUE, NS_GOTFOCUS, this);
#ifdef DEBUG  
  printf("send NS_GOTFOCUS from nsWindow::OnFocusInSignal\n");
#endif




  AddRef();
  
  DispatchFocus(event);
  
  Release();
}

 void
nsWindow::OnFocusOutSignal(GdkEventFocus * aGdkFocusEvent)
{

  GTK_WIDGET_UNSET_FLAGS(mMozArea, GTK_HAS_FOCUS);

  nsFocusEvent event(PR_TRUE, NS_LOSTFOCUS, this);
  



  AddRef();
  
  DispatchFocus(event);
  
  Release();
}


void 
nsWindow::InstallFocusInSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"focus_in_event",
				GTK_SIGNAL_FUNC(nsWindow::FocusInSignal));
}

void 
nsWindow::InstallFocusOutSignal(GtkWidget * aWidget)
{
  NS_ASSERTION( nsnull != aWidget, "widget is null");

  InstallSignal(aWidget,
				(gchar *)"focus_out_event",
				GTK_SIGNAL_FUNC(nsWindow::FocusOutSignal));
}


void 
nsWindow::HandleGDKEvent(GdkEvent *event)
{
  if (mIsDestroying)
    return;

  switch (event->any.type)
  {
  case GDK_MOTION_NOTIFY:
    
    
    sIsDraggingOutOf = PR_FALSE;
    OnMotionNotifySignal (&event->motion);
    break;
  case GDK_BUTTON_PRESS:
  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
    OnButtonPressSignal (&event->button);
    break;
  case GDK_BUTTON_RELEASE:
    OnButtonReleaseSignal (&event->button);
    break;
  case GDK_ENTER_NOTIFY:
    if(is_parent_ungrab_enter(&event->crossing))
      return;

    OnEnterNotifySignal (&event->crossing);
    break;
  case GDK_LEAVE_NOTIFY:
    if(is_parent_grab_leave(&event->crossing))
      return;

    OnLeaveNotifySignal (&event->crossing);
    break;

  default:
    break;
  }
}

void
nsWindow::OnDestroySignal(GtkWidget* aGtkWidget)
{
  nsWidget::OnDestroySignal(aGtkWidget);
  if (aGtkWidget == mShell) {
    mShell = nsnull;
  }
}

gint handle_delete_event(GtkWidget *w, GdkEventAny *e, nsWindow *win)
{

  PRBool isEnabled;
  
  win->IsEnabled(&isEnabled);
  if (!isEnabled)
    return TRUE;

  NS_ADDREF(win);

  
  nsGUIEvent event(PR_TRUE, NS_XUL_CLOSE, win);
  nsEventStatus status;
  
  win->DispatchEvent(&event, status);

  NS_RELEASE(win);
  return TRUE;
}



NS_IMETHODIMP nsWindow::PreCreateWidget(nsWidgetInitData *aInitData)
{
  if (nsnull != aInitData) {
    SetWindowType(aInitData->mWindowType);
    SetBorderStyle(aInitData->mBorderStyle);

    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}


gint nsWindow::ConvertBorderStyles(nsBorderStyle bs)
{
  gint w = 0;

  if (bs == eBorderStyle_default)
    return -1;

  if (bs & eBorderStyle_all)
    w |= GDK_DECOR_ALL;
  if (bs & eBorderStyle_border)
    w |= GDK_DECOR_BORDER;
  if (bs & eBorderStyle_resizeh)
    w |= GDK_DECOR_RESIZEH;
  if (bs & eBorderStyle_title)
    w |= GDK_DECOR_TITLE;
  if (bs & eBorderStyle_menu)
    w |= GDK_DECOR_MENU;
  if (bs & eBorderStyle_minimize)
    w |= GDK_DECOR_MINIMIZE;
  if (bs & eBorderStyle_maximize)
    w |= GDK_DECOR_MAXIMIZE;
  if (bs & eBorderStyle_close) {
#ifdef DEBUG
    printf("we don't handle eBorderStyle_close yet... please fix me\n");
#endif 
  }

  return w;
}








NS_METHOD nsWindow::CreateNative(GtkObject *parentWidget)
{
  GdkSuperWin  *superwin = 0;
  GdkEventMask  mask;
  PRBool        parentIsContainer = PR_FALSE;
  GtkContainer *parentContainer = NULL;
  GtkWindow    *topLevelParent  = NULL;

  if (parentWidget) {
    if (GDK_IS_SUPERWIN(parentWidget)) {
      superwin = GDK_SUPERWIN(parentWidget);
      GdkWindow *topGDKWindow =
        gdk_window_get_toplevel(GDK_SUPERWIN(parentWidget)->shell_window);
      gpointer data;
      gdk_window_get_user_data(topGDKWindow, &data);
      if (GTK_IS_WINDOW(data)) {
        topLevelParent = GTK_WINDOW(data);
      }
    }
    else if (GTK_IS_CONTAINER(parentWidget)) {
      parentContainer = GTK_CONTAINER(parentWidget);
      parentIsContainer = PR_TRUE;
      topLevelParent =
        GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(parentWidget)));
    }
    else {
      NS_WARNING("Unknown parent widget type");
    }
  }

  switch(mWindowType)
  {
  case eWindowType_dialog:
    mIsToplevel = PR_TRUE;
    mShell = gtk_window_new(GTK_WINDOW_DIALOG);
    if (topLevelParent) {
      gtk_window_set_transient_for(GTK_WINDOW(mShell), topLevelParent);
      mTransientParent = topLevelParent;
    }
    gtk_window_set_policy(GTK_WINDOW(mShell), PR_TRUE, PR_TRUE, PR_FALSE);
    
    InstallRealizeSignal(mShell);

    
    
    mMozArea = gtk_mozarea_new();
    gtk_container_add(GTK_CONTAINER(mShell), mMozArea);
    gtk_widget_realize(GTK_WIDGET(mMozArea));
    SetIcon(NS_LITERAL_STRING("default"));
    mSuperWin = GTK_MOZAREA(mMozArea)->superwin;
    
    
    gdk_window_set_back_pixmap(mShell->window, NULL, FALSE);

    gtk_signal_connect(GTK_OBJECT(mShell),
                       "delete_event",
                       GTK_SIGNAL_FUNC(handle_delete_event),
                       this);
    break;

  case eWindowType_popup:
    mIsToplevel = PR_TRUE;
    mShell = gtk_window_new(GTK_WINDOW_POPUP);
    if (topLevelParent) {
      gtk_window_set_transient_for(GTK_WINDOW(mShell), topLevelParent);
      mTransientParent = topLevelParent;
    }
    
    
    mMozArea = gtk_mozarea_new();
    gtk_container_add(GTK_CONTAINER(mShell), mMozArea);
    gtk_widget_realize(GTK_WIDGET(mMozArea));
    mSuperWin = GTK_MOZAREA(mMozArea)->superwin;
    
    
    gdk_window_set_back_pixmap(mShell->window, NULL, FALSE);

    
    

    mCursor = eCursor_wait; 
                            
                            
    SetCursor(eCursor_standard);

    break;

  case eWindowType_toplevel:
  case eWindowType_invisible:
    mIsToplevel = PR_TRUE;
    mShell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    gtk_window_set_policy(GTK_WINDOW(mShell), PR_TRUE, PR_TRUE, PR_FALSE);
    InstallRealizeSignal(mShell);
    
    
    mMozArea = gtk_mozarea_new();
    gtk_container_add(GTK_CONTAINER(mShell), mMozArea);
    gtk_widget_realize(GTK_WIDGET(mMozArea));
    SetIcon(NS_LITERAL_STRING("default"));
    mSuperWin = GTK_MOZAREA(mMozArea)->superwin;
    
    
    gdk_window_set_back_pixmap(mShell->window, NULL, FALSE);

    if (!topLevelParent) {
      GdkWindow* dialoglead = mShell->window;
      gdk_window_set_group(dialoglead, dialoglead);
    }
    gtk_signal_connect(GTK_OBJECT(mShell),
                       "delete_event",
                       GTK_SIGNAL_FUNC(handle_delete_event),
                       this);
    gtk_signal_connect_after(GTK_OBJECT(mShell),
                             "size_allocate",
                             GTK_SIGNAL_FUNC(handle_size_allocate),
                             this);
    break;

  case eWindowType_child:
    
    
    if (parentIsContainer) {
      
      if (!GTK_WIDGET_REALIZED(parentContainer))
        g_print("Warning: The parent container of this widget is not realized.  I'm going to crash very, very soon.\n");
      else {
        
        
        mMozArea = gtk_mozarea_new();
        gtk_container_add(parentContainer, mMozArea);
        gtk_widget_realize(GTK_WIDGET(mMozArea));
        mSuperWin = GTK_MOZAREA(mMozArea)->superwin;
      }
    }
    else {
      if (superwin) {
        mSuperWin = gdk_superwin_new(superwin->bin_window,
                                     mBounds.x, mBounds.y,
                                     mBounds.width, mBounds.height);
        nsWindow* realParent =
          NS_STATIC_CAST(nsWindow*, gtk_object_get_data(GTK_OBJECT(superwin), "nsWindow"));
        if (realParent && !mParent) {
          realParent->mHasAnonymousChildren = PR_TRUE;
        }
      }
    }
    break;

  default:
    break;
  }

  

  if (mShell) {
    mask = (GdkEventMask) (GDK_KEY_PRESS_MASK |
                           GDK_KEY_RELEASE_MASK |
                           GDK_FOCUS_CHANGE_MASK );
    gdk_window_set_events(mShell->window, 
                          mask);

  }

  if (mMozArea) {
    
    GTK_WIDGET_SET_FLAGS(mMozArea, GTK_CAN_FOCUS);
    
    
    
    
    
    if (mShell)
      gtk_window_set_focus(GTK_WINDOW(mShell), mMozArea);

    
    gtk_signal_connect(GTK_OBJECT(mMozArea),
                       "focus_in_event",
                       GTK_SIGNAL_FUNC(handle_mozarea_focus_in),
                       this);
    gtk_signal_connect(GTK_OBJECT(mMozArea),
                       "focus_out_event",
                       GTK_SIGNAL_FUNC(handle_mozarea_focus_out),
                       this);
    
    
    InstallButtonPressSignal(mMozArea);
    InstallButtonReleaseSignal(mMozArea);
  }

  

  mask = (GdkEventMask)(GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_EXPOSURE_MASK |
                        GDK_FOCUS_CHANGE_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_KEY_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK);


  NS_ASSERTION(mSuperWin,"no super window!");
  if (!mSuperWin) return NS_ERROR_FAILURE;

  gdk_window_set_events(mSuperWin->bin_window, 
                        mask);

  
  gtk_object_set_data (GTK_OBJECT (mSuperWin), "nsWindow", this);
  
  
  if (mShell)
    gtk_object_set_data(GTK_OBJECT(mShell), "nsWindow", this);
  if (mMozArea) {
    gtk_object_set_data(GTK_OBJECT(mMozArea), "nsWindow", this);
  }
  
  gdk_window_set_user_data (mSuperWin->bin_window, (gpointer)mSuperWin);

  
  gdk_window_set_back_pixmap(mSuperWin->bin_window, NULL, 0);

  if (mShell) {
    
    gtk_drag_dest_set(mShell,
                      (GtkDestDefaults)0,
                      NULL,
                      0,
                      (GdkDragAction)0);

    gtk_signal_connect(GTK_OBJECT(mShell),
                       "drag_motion",
                       GTK_SIGNAL_FUNC(nsWindow::DragMotionSignal),
                       this);
    gtk_signal_connect(GTK_OBJECT(mShell),
                       "drag_leave",
                       GTK_SIGNAL_FUNC(nsWindow::DragLeaveSignal),
                       this);
    gtk_signal_connect(GTK_OBJECT(mShell),
                       "drag_drop",
                       GTK_SIGNAL_FUNC(nsWindow::DragDropSignal),
                       this);
    gtk_signal_connect(GTK_OBJECT(mShell),
                       "drag_data_received",
                       GTK_SIGNAL_FUNC(nsWindow::DragDataReceived),
                       this);

    
    gtk_signal_connect(GTK_OBJECT(mShell),
                       "client_event",
                       GTK_SIGNAL_FUNC(nsWindow::ClientEventSignal),
                       this);
  }

  if (mMozArea) {

    
    
    mask = (GdkEventMask) ( GDK_EXPOSURE_MASK |
                           GDK_KEY_PRESS_MASK |
                           GDK_KEY_RELEASE_MASK |
                           GDK_ENTER_NOTIFY_MASK |
                           GDK_LEAVE_NOTIFY_MASK |
                           GDK_STRUCTURE_MASK | 
                           GDK_FOCUS_CHANGE_MASK );
    gdk_window_set_events(mMozArea->window, 
                          mask);
    gtk_signal_connect(GTK_OBJECT(mMozArea),
                       "key_press_event",
                       GTK_SIGNAL_FUNC(handle_key_press_event),
                       this);
    gtk_signal_connect(GTK_OBJECT(mMozArea),
                       "key_release_event",
                       GTK_SIGNAL_FUNC(handle_key_release_event),
                       this);

    
    
    
    
    gtk_signal_connect(GTK_OBJECT(mMozArea),
                       "toplevel_configure",
                       GTK_SIGNAL_FUNC(handle_toplevel_configure),
                       this);
  }

  if (mSuperWin) {
    
    
    
    g_hash_table_insert(mWindowLookupTable, mSuperWin->shell_window, this);
  }

  
  
  GtkWidget *top_mozarea = GetOwningWidget();
  if (top_mozarea) {
    gtk_signal_connect_while_alive(GTK_OBJECT(top_mozarea),
                                   "toplevel_configure",
                                   GTK_SIGNAL_FUNC(handle_invalidate_pos),
                                   this,
                                   GTK_OBJECT(mSuperWin));
  }

  return NS_OK;
}






void nsWindow::InitCallbacks(char * aName)
{
  NS_ASSERTION(mSuperWin,"no superwin, can't init callbacks");
  if (mSuperWin) {
    gdk_superwin_set_event_funcs(mSuperWin,
                                 handle_xlib_shell_event,
                                 handle_superwin_paint,
                                 handle_superwin_flush,
                                 nsXKBModeSwitch::HandleKeyPress,
                                 nsXKBModeSwitch::HandleKeyRelease,
                                 this, NULL);
  }
}






void * nsWindow::GetNativeData(PRUint32 aDataType)
{

  if (aDataType == NS_NATIVE_WINDOW)
  {
    if (mSuperWin) {
      GdkWindowPrivate *private_window = (GdkWindowPrivate *)mSuperWin->bin_window;
      if (private_window->destroyed == PR_TRUE) {
        return NULL;
      }
      return (void *)mSuperWin->bin_window;
    }
  }
  else if (aDataType == NS_NATIVE_WIDGET) {
    if (mSuperWin) {
      GdkWindowPrivate *private_window = (GdkWindowPrivate *)mSuperWin->bin_window;
      if (private_window->destroyed == PR_TRUE) {
        return NULL;
      }
    }
    return (void *)mSuperWin;
  }
  else if (aDataType == NS_NATIVE_PLUGIN_PORT) {
    if (mSuperWin) {
      GdkWindowPrivate *private_window = (GdkWindowPrivate *)mSuperWin->bin_window;
      if (private_window->destroyed == PR_TRUE) {
        return NULL;
      }

      
      
      
      XSync(GDK_DISPLAY(), False);
      return (void *)GDK_WINDOW_XWINDOW(mSuperWin->bin_window);
    }
    return NULL;
  }
  else if (aDataType == NS_NATIVE_SHELLWIDGET) {
    return (void *) mShell;
  }

  return nsWidget::GetNativeData(aDataType);
}







NS_IMETHODIMP nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  NS_ASSERTION(mIsDestroying != PR_TRUE, "Trying to scroll a destroyed widget\n");
  UnqueueDraw();
  mUpdateArea->Offset(aDx, aDy);

  if (mSuperWin) {
    
    gdk_superwin_scroll(mSuperWin, aDx, aDy);
  }

  
  for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
    nsRect bounds;
    kid->GetBounds(bounds);
    bounds.x += aDx;
    bounds.y += aDy;
    nsWidget* childWidget = NS_STATIC_CAST(nsWidget*, kid);
    childWidget->SetBounds(bounds);
    childWidget->ResetInternalVisibility();
  }

  
  InvalidateWindowPos();

  return NS_OK;
}






NS_IMETHODIMP nsWindow::ScrollWidgets(PRInt32 aDx, PRInt32 aDy)
{
  UnqueueDraw();
  mUpdateArea->Offset(aDx, aDy);

  if (mSuperWin) {
    
    gdk_superwin_scroll(mSuperWin, aDx, aDy);
  }
  return NS_OK;
}

NS_IMETHODIMP nsWindow::ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy)
{
  return NS_OK;
}

NS_IMETHODIMP nsWindow::SetTitle(const nsAString& aTitle)
{
  if (!mShell)
    return NS_ERROR_FAILURE;

  nsresult rv;
  char *platformText = nsnull;
  PRInt32 platformLen;

  
#define UTF8_FOLLOWBYTE(ch) (((ch) & 0xC0) == 0x80)
  NS_ConvertUTF16toUTF8 titleUTF8(aTitle);
  if (titleUTF8.Length() > NS_WINDOW_TITLE_MAX_LENGTH) {
    
    
    PRUint32 len = NS_WINDOW_TITLE_MAX_LENGTH;
    while(UTF8_FOLLOWBYTE(titleUTF8[len]))
      --len;
    titleUTF8.Truncate(len);
  }

  XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(mShell->window),
                XInternAtom(GDK_DISPLAY(), "_NET_WM_NAME", False),
                XInternAtom(GDK_DISPLAY(), "UTF8_STRING", False),
                8, PropModeReplace, (unsigned char *) titleUTF8.get(),
                titleUTF8.Length());

  
  XChangeProperty(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(mShell->window),
                XInternAtom(GDK_DISPLAY(), "_NET_WM_ICON_NAME", False),
                XInternAtom(GDK_DISPLAY(), "UTF8_STRING", False),
                8, PropModeReplace, (unsigned char *) titleUTF8.get(),
                titleUTF8.Length());

  nsCOMPtr<nsIUnicodeEncoder> encoder;
  
  nsCAutoString platformCharset;
  nsCOMPtr <nsIPlatformCharset> platformCharsetService = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv))
    rv = platformCharsetService->GetCharset(kPlatformCharsetSel_Menu, platformCharset);

  
  if (NS_FAILED(rv))
    platformCharset.AssignLiteral("ISO-8859-1");

  
  nsCOMPtr<nsICharsetConverterManager> ccm = 
           do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);  
  rv = ccm->GetUnicodeEncoderRaw(platformCharset.get(), getter_AddRefs(encoder));
  NS_ASSERTION(NS_SUCCEEDED(rv), "GetUnicodeEncoderRaw failed.");
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  
  
  nsAString::const_iterator begin;
  const PRUnichar *title = aTitle.BeginReading(begin).get();
  PRInt32 len = (PRInt32)aTitle.Length();
  encoder->GetMaxLength(title, len, &platformLen);
  if (platformLen) {
    
    if (platformLen > NS_WINDOW_TITLE_MAX_LENGTH) {
      platformLen = NS_WINDOW_TITLE_MAX_LENGTH;
    }
    platformText = NS_REINTERPRET_CAST(char*, nsMemory::Alloc(platformLen + sizeof(char)));
    if (platformText) {
      rv = encoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, '?');
      if (NS_SUCCEEDED(rv))
        rv = encoder->Convert(title, &len, platformText, &platformLen);
      (platformText)[platformLen] = '\0';  
    }
  } 

  if (platformLen > 0 && platformText) {
    gtk_window_set_title(GTK_WINDOW(mShell), platformText);
  }
  else {
    gtk_window_set_title(GTK_WINDOW(mShell), "");
  }

  if (platformText)
    nsMemory::Free(platformText);

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::SetIcon(const nsAString& aIconSpec)
{
  
  
  NS_ConvertUTF16toUTF8 iconKey(aIconSpec);
  IconEntry* entry = NS_STATIC_CAST(IconEntry*,
                                    PL_DHashTableOperate(sIconCache,
                                                         iconKey.get(),
                                                         PL_DHASH_ADD));
  if (!entry)
      return NS_ERROR_OUT_OF_MEMORY;

  if (!entry->string) {
    
#ifdef NS_DEBUG
    PRUint32 generation = sIconCache->generation;
#endif

    GtkStyle* w_style;
    GdkPixmap* w_pixmap = NULL, *w_minipixmap = NULL;
    GdkBitmap* w_mask = NULL, *w_minimask = NULL;

    w_style = gtk_widget_get_style(mShell);

    nsCOMPtr<nsILocalFile> iconFile;
    ResolveIconName(aIconSpec, NS_LITERAL_STRING(".xpm"),
                    getter_AddRefs(iconFile));
    if (iconFile) {
      nsCAutoString path;
      iconFile->GetNativePath(path);

      w_pixmap =
        gdk_pixmap_colormap_create_from_xpm(mShell->window,
                                            gdk_colormap_get_system(),
                                            &w_mask,
                                            &w_style->bg[GTK_STATE_NORMAL],
                                            path.get());
#ifdef DEBUG_ICONS
      printf("Loaded large icon file: %s\n", path.get());
#endif
    }

    ResolveIconName(aIconSpec, NS_LITERAL_STRING("16.xpm"),
                    getter_AddRefs(iconFile));
    if (iconFile) {
      nsCAutoString path;
      iconFile->GetNativePath(path);

      w_minipixmap =
        gdk_pixmap_colormap_create_from_xpm(mShell->window,
                                            gdk_colormap_get_system(),
                                            &w_minimask,
                                            &w_style->bg[GTK_STATE_NORMAL],
                                            path.get());
#ifdef DEBUG_ICONS
      printf("Loaded small icon file: %s\n", path.get());
#endif
    }

    NS_ASSERTION(sIconCache->generation == generation, "sIconCache changed!");
    entry->string = strdup(iconKey.get());
    entry->w_pixmap = w_pixmap;
    entry->w_mask = w_mask;
    entry->w_minipixmap = w_minipixmap;
    entry->w_minimask = w_minimask;
  }
#ifdef DEBUG_ICONS
  else
    printf("Loaded icon set for %s from cache\n", iconKey.get());
#endif

  if (entry->w_pixmap && SetIcon(entry->w_pixmap, entry->w_mask) != NS_OK)
    return NS_ERROR_FAILURE;

  
  if (entry->w_minipixmap)
    return SetMiniIcon (entry->w_minipixmap, entry->w_minimask);
  return NS_OK;
}

nsresult nsWindow::SetMiniIcon(GdkPixmap *pixmap,
                               GdkBitmap *mask)
{
   GdkAtom icon_atom;
   glong data[2];

   if (!mShell)
      return NS_ERROR_FAILURE;
   
   data[0] = ((GdkPixmapPrivate *)pixmap)->xwindow;
   data[1] = ((GdkPixmapPrivate *)mask)->xwindow;

   icon_atom = gdk_atom_intern ("KWM_WIN_ICON", FALSE);
   gdk_property_change (mShell->window, icon_atom, icon_atom,
                        32, GDK_PROP_MODE_REPLACE,
                        (guchar *)data, 2);
   return NS_OK;
}


nsresult nsWindow::SetIcon(GdkPixmap *pixmap, 
                           GdkBitmap *mask)
{
  if (!mShell)
    return NS_ERROR_FAILURE;

  gdk_window_set_icon(mShell->window, (GdkWindow*)nsnull, pixmap, mask);

  return NS_OK;
}

NS_IMETHODIMP nsWindow::BeginResizingChildren(void)
{
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::EndResizingChildren(void)
{
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::GetScreenBounds(nsRect &aRect)
{
  nsRect origin(0,0,mBounds.width,mBounds.height);
  WidgetToScreen(origin, aRect);
  return NS_OK;
}








NS_IMETHODIMP nsWindow::Show(PRBool bState)
{
  if (!mSuperWin)
    return NS_OK; 

  mShown = bState;

  
  ResetInternalVisibility();

  return NS_OK;
}

void nsWindow::ResetInternalVisibility()
{
  if (mShell)
  { 
    SetInternalVisibility(mShown);
  }
  else
  {
    nsWidget::ResetInternalVisibility();
  }
}

void nsWindow::SetInternalVisibility(PRBool aVisible)
{
  
  if (mIsTooSmall)
  {
    aVisible = PR_FALSE;
  }

  
  if (aVisible == mInternalShown)
  {
    return;
  }

  mInternalShown = aVisible;

  if (aVisible)
  {
    
    
    
    
    
    
    
    if (mIsTranslucent) {
      ApplyTransparencyBitmap();
    }

    
    gdk_window_show(mSuperWin->bin_window);
    gdk_window_show(mSuperWin->shell_window);

    if (mMozArea)
    {
      gtk_widget_show(mMozArea);
      
      if (mShell)
        gtk_widget_show(mShell);
    }

    
    
    if (GetNextSibling()) {
      ResetZOrder();
    }

    
    if (sGrabWindow == this && mLastGrabFailed && !nsWindow::DragInProgress())
      NativeGrab(PR_TRUE);
  }
  
  else
  {
    gdk_window_hide(mSuperWin->bin_window);
    gdk_window_hide(mSuperWin->shell_window);
    

    
    if (mMozArea)
    {
      
      if (mShell)
        gtk_widget_hide(mShell);
      gtk_widget_hide(mMozArea);
    } 

  }
}






NS_IMETHODIMP nsWindow::CaptureMouse(PRBool aCapture)
{
  GtkWidget *grabWidget;

  if (mIsToplevel && mMozArea)
    grabWidget = mMozArea;
  else
    grabWidget = mWidget;

  if (aCapture)
  {
    if (!grabWidget) {
#ifdef DEBUG
      g_print("nsWindow::CaptureMouse on NULL grabWidget\n");
#endif
      return NS_ERROR_FAILURE;
    }

    GdkCursor *cursor = gdk_cursor_new (GDK_ARROW);
    DropMotionTarget();
    gdk_pointer_grab (GTK_WIDGET(grabWidget)->window, PR_TRUE, (GdkEventMask)
                      (GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                       GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
                       GDK_POINTER_MOTION_MASK),
                      (GdkWindow*) NULL, cursor, GDK_CURRENT_TIME);
    gdk_cursor_destroy(cursor);
    gtk_grab_add(grabWidget);
  }
  else
  {
    DropMotionTarget();
    gdk_pointer_ungrab(GDK_CURRENT_TIME);
    if (grabWidget) gtk_grab_remove(grabWidget);
  }

  return NS_OK;
}

NS_IMETHODIMP nsWindow::ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY)
{
  if (mIsToplevel && mShell)
  {
    PRInt32 screenWidth = gdk_screen_width();
    PRInt32 screenHeight = gdk_screen_height();
    if (aAllowSlop) {
      if (*aX < kWindowPositionSlop - mBounds.width)
        *aX = kWindowPositionSlop - mBounds.width;
      if (*aX > screenWidth - kWindowPositionSlop)
        *aX = screenWidth - kWindowPositionSlop;
      if (*aY < kWindowPositionSlop - mBounds.height)
        *aY = kWindowPositionSlop - mBounds.height;
      if (*aY > screenHeight - kWindowPositionSlop)
        *aY = screenHeight - kWindowPositionSlop;
    } else {
      if (*aX < 0)
        *aX = 0;
      if (*aX > screenWidth - mBounds.width)
        *aX = screenWidth - mBounds.width;
      if (*aY < 0)
        *aY = 0;
      if (*aY > screenHeight - mBounds.height)
        *aY = screenHeight - mBounds.height;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
  InvalidateWindowPos();        
  
  if((aX == mBounds.x) && (aY == mBounds.y) && !mIsToplevel) {
     return NS_OK;
  }

  mBounds.x = aX;
  mBounds.y = aY;

  if (mIsToplevel && mShell)
  {
#ifdef DEBUG
    

    if (!mParent) {
      PRInt32 screenWidth = gdk_screen_width();
      PRInt32 screenHeight = gdk_screen_height();
      
      if (aX < 0 || aX >= screenWidth || aY < 0 || aY >= screenHeight)
        printf("window moved to offscreen position\n");
    }
#endif

    
    if (mParent && mWindowType == eWindowType_popup) {
      
      nsRect oldrect, newrect;
      oldrect.x = aX;
      oldrect.y = aY;
      mParent->WidgetToScreen(oldrect, newrect);
      gtk_widget_set_uposition(mShell, newrect.x, newrect.y);
    } else {
      gtk_widget_set_uposition(mShell, aX, aY);
    }
  }
  else if (mSuperWin) {
    gdk_window_move(mSuperWin->shell_window, aX, aY);
  }

  ResetInternalVisibility();

  return NS_OK;
}

NS_IMETHODIMP nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  PRInt32 sizeHeight = aHeight;
  PRInt32 sizeWidth = aWidth;

#if 0
  printf("nsWindow::Resize %s (%p) to %d %d\n",
         (const char *) debug_GetName(mWidget),
         this,
         aWidth, aHeight);
#endif
  
  ResizeTransparencyBitmap(aWidth, aHeight);

  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  
  
  
  
  if (aWidth <= 1 || aHeight <= 1)
  {
    mIsTooSmall = PR_TRUE;
  }
  else
  {
    mIsTooSmall = PR_FALSE;
  }

  if (mSuperWin) {
    
    if (mIsToplevel && mShell)
    {
      
      if (GTK_WIDGET_VISIBLE(mShell) && GTK_WIDGET_REALIZED(mShell))  
        gdk_window_resize(mShell->window, aWidth, aHeight);

      gtk_window_set_default_size(GTK_WINDOW(mShell), aWidth, aHeight);
    }
    
    else if (mMozArea)
      gdk_window_resize(mMozArea->window, aWidth, aHeight);
    
    gdk_superwin_resize(mSuperWin, aWidth, aHeight);
  }
  if (mIsToplevel || mListenForResizes) {
    
    nsSizeEvent sevent(PR_TRUE, NS_SIZE, this);
    sevent.windowSize = new nsRect (0, 0, sizeWidth, sizeHeight);
    sevent.mWinWidth = sizeWidth;
    sevent.mWinHeight = sizeHeight;
    
    AddRef();
    OnResize(&sevent);
    Release();
    delete sevent.windowSize;
  }
  else {
    
  }

  if (aRepaint)
    Invalidate(PR_FALSE);

  
  ResetInternalVisibility();
  for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
    NS_STATIC_CAST(nsWidget*, kid)->ResetInternalVisibility();
  }

  return NS_OK;
}

NS_IMETHODIMP nsWindow::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                               PRInt32 aHeight, PRBool aRepaint)
{
  Move(aX,aY);
  
  Resize(aWidth,aHeight,aRepaint);
  return NS_OK;
}

NS_IMETHODIMP nsWindow::GetAttention(PRInt32 aCycleCount)
{
  
  GtkWidget *top_mozarea = GetOwningWidget();
  if (top_mozarea) {
    GtkWidget *top_window = gtk_widget_get_toplevel(top_mozarea);
    if (top_window && GTK_WIDGET_VISIBLE(top_window)) {
      
      gdk_window_show(top_window->window);
    }
  }
  return NS_OK;
}

 void
nsWindow::OnRealize(GtkWidget *aWidget)
{
  if (aWidget == mShell) {
    gint wmd = ConvertBorderStyles(mBorderStyle);
    if (wmd != -1)
      gdk_window_set_decorations(mShell->window, (GdkWMDecoration)wmd);
  }
}

gint handle_mozarea_focus_in(GtkWidget *      aWidget, 
                             GdkEventFocus *  aGdkFocusEvent, 
                             gpointer         aData)
{
  if (!aWidget)
    return FALSE;

  if (!aGdkFocusEvent)
    return FALSE;

  nsWindow *widget = (nsWindow *)aData;

  if (!widget)
    return FALSE;

#ifdef DEBUG_FOCUS
  printf("handle_mozarea_focus_in\n");
#endif

#ifdef DEBUG_FOCUS
  printf("aWidget is %p\n", NS_STATIC_CAST(void *, aWidget));
#endif

  
  GTK_WIDGET_SET_FLAGS(aWidget, GTK_HAS_FOCUS);

  widget->HandleMozAreaFocusIn();

  return FALSE;
}

gint handle_mozarea_focus_out(GtkWidget *      aWidget, 
                              GdkEventFocus *  aGdkFocusEvent, 
                              gpointer         aData)
{
#ifdef DEBUG_FOCUS
  printf("handle_mozarea_focus_out\n");
#endif

  if (!aWidget) {
    return FALSE;
  }
  
  if (!aGdkFocusEvent) {
    return FALSE;
  }

  nsWindow *widget = (nsWindow *) aData;

  if (!widget) {
    return FALSE;
  }

  
  GTK_WIDGET_UNSET_FLAGS(aWidget, GTK_HAS_FOCUS);

  widget->HandleMozAreaFocusOut();

  return FALSE;
}

void handle_toplevel_configure (
    GtkMozArea *      aArea,
    nsWindow   *      aWindow)
{
  

  
  nsRect oldBounds;
  aWindow->GetBounds(oldBounds);

  
  
  nscoord x,y;
  gdk_window_get_origin(GTK_WIDGET(aArea)->window, &x, &y);

  if ((oldBounds.x == x) && (oldBounds.y == y)) {
    return;
  }

  aWindow->OnMove(x, y);
}

void
nsWindow::HandleXlibConfigureNotifyEvent(XEvent *event)
{
#if 0
  XEvent    config_event;

  while (XCheckTypedWindowEvent(event->xany.display, 
                                event->xany.window, 
                                ConfigureNotify,
                                &config_event) == True) {
    
    
    
    
    *event = config_event;
    
    
    
#if 0
    g_print("Extra ConfigureNotify event for window 0x%lx %d %d %d %d\n",
            event->xconfigure.window,
            event->xconfigure.x, 
            event->xconfigure.y,
            event->xconfigure.width, 
            event->xconfigure.height);
#endif
  }

  

#endif

  if (mIsToplevel) {
    nsSizeEvent sevent(PR_TRUE, NS_SIZE, this);
    sevent.windowSize = new nsRect (event->xconfigure.x, event->xconfigure.y,
                                    event->xconfigure.width, event->xconfigure.height);
    sevent.refPoint.x = event->xconfigure.x;
    sevent.refPoint.y = event->xconfigure.y;
    sevent.mWinWidth = event->xconfigure.width;
    sevent.mWinHeight = event->xconfigure.height;
    
    AddRef();
    OnResize(&sevent);
    Release();
    delete sevent.windowSize;
  }
}


GtkWidget *
nsWindow::GetOwningWidget()
{
  GdkWindow *parent = nsnull;
  GtkWidget *widget;

  if (mMozAreaClosestParent)
  {
    return (GtkWidget *)mMozAreaClosestParent;
  }
  if ((mMozAreaClosestParent == nsnull) && mMozArea)
  {
    mMozAreaClosestParent = mMozArea;
    return (GtkWidget *)mMozAreaClosestParent;
  }
  
  if (mSuperWin)
  {
    parent = mSuperWin->shell_window;
  }

  while (parent)
  {
    gdk_window_get_user_data (parent, (void **)&widget);
    if (widget != nsnull && GTK_IS_MOZAREA (widget))
    {
      mMozAreaClosestParent = widget;
      break;
    }
    parent = gdk_window_get_parent (parent);
    parent = gdk_window_get_parent (parent);
  }
  
  return (GtkWidget *)mMozAreaClosestParent;
}

nsWindow *
nsWindow::GetOwningWindow(void) 
{
  GtkWidget *widget = GetOwningWidget();

  return NS_STATIC_CAST(nsWindow *, gtk_object_get_data(GTK_OBJECT(widget),
                                                        "nsWindow"));
}

nsWindowType
nsWindow::GetOwningWindowType(void)
{
  nsWindow *owningWindow;
  owningWindow = GetOwningWindow();

  nsWindowType retval;
  owningWindow->GetWindowType(retval);

  return retval;
}

PRBool
nsWindow::GrabInProgress(void)
{
  return nsWindow::sIsGrabbing;
}


PRBool
nsWindow::DragInProgress(void)
{
  
  
  
  if (mLastDragMotionWindow || sIsDraggingOutOf)
    return PR_TRUE;
  else
    return PR_FALSE;
}



nsWindow *
nsWindow::GetGrabWindow(void)
{
  if (nsWindow::sIsGrabbing)
    return sGrabWindow;
  else
    return nsnull;
}

GdkWindow *
nsWindow::GetGdkGrabWindow(void)
{
  if (!nsWindow::sIsGrabbing)
  {
    return nsnull;
  }
  if (mTransientParent)
    return GTK_WIDGET(mTransientParent)->window;
  else
    return mSuperWin->bin_window;

}

GdkWindow *
nsWindow::GetLayeringWindow()
{
  return mSuperWin->shell_window;
}

 GdkWindow *
nsWindow::GetRenderWindow(GtkObject * aGtkObject)
{
  GdkWindow * renderWindow = nsnull;

  if (aGtkObject)
  {
    if (GDK_IS_SUPERWIN(aGtkObject))
    {
      renderWindow = GDK_SUPERWIN(aGtkObject)->bin_window;
    }
  }
  return renderWindow;
}


GtkWindow *nsWindow::GetTopLevelWindow(void)
{
  GtkWidget *moz_area;

  if (!mSuperWin)
    return NULL;
  moz_area = GetOwningWidget();
  return GTK_WINDOW(gtk_widget_get_toplevel(moz_area));
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
  
  
  if (aDragContext->actions & GDK_ACTION_DEFAULT) {
    action = nsIDragService::DRAGDROP_ACTION_MOVE;
  }

  
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


gint
nsWindow::DragMotionSignal (GtkWidget *      aWidget,
                            GdkDragContext   *aDragContext,
                            gint             aX,
                            gint             aY,
                            guint            aTime,
                            void             *aData)
{
  nsWindow *window = (nsWindow *)aData;
  return window->OnDragMotionSignal(aWidget, aDragContext, aX, aY, aTime, aData);
}

gint nsWindow::OnDragMotionSignal      (GtkWidget *      aWidget,
                                        GdkDragContext   *aDragContext,
                                        gint             aX,
                                        gint             aY,
                                        guint            aTime,
                                        void             *aData)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::OnDragMotionSignal\n");
#endif 

  sIsDraggingOutOf = PR_FALSE;

  
  ResetDragMotionTimer(aWidget, aDragContext, aX, aY, aTime);

  
  nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
  nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

  
  
  nscoord retx = 0;
  nscoord rety = 0;

  Window thisWindow = GDK_WINDOW_XWINDOW(aWidget->window);
  Window returnWindow = None;
  returnWindow = GetInnerMostWindow(thisWindow, thisWindow, aX, aY,
                                    &retx, &rety, 0);

  nsWindow *innerMostWidget = NULL;
  innerMostWidget = GetnsWindowFromXWindow(returnWindow);

  if (!innerMostWidget)
    innerMostWidget = this;

  
  if (mLastDragMotionWindow) {
    
    if (mLastDragMotionWindow != innerMostWidget) {
      
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

  nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, innerMostWidget,
                     nsMouseEvent::eReal);

  InitDragEvent(event);

  
  UpdateDragStatus(event, aDragContext, dragService);

  event.refPoint.x = retx;
  event.refPoint.y = rety;

  innerMostWidget->AddRef();

  innerMostWidget->DispatchMouseEvent(event);

  innerMostWidget->Release();

  
  dragSessionGTK->TargetEndDragMotion(aWidget, aDragContext, aTime);
  
  
  dragSessionGTK->TargetSetLastContext(0, 0, 0);

  return TRUE;
}


void
nsWindow::DragLeaveSignal  (GtkWidget *      aWidget,
                            GdkDragContext   *aDragContext,
                            guint            aTime,
                            gpointer         aData)
{
  nsWindow *window = (nsWindow *)aData;
  window->OnDragLeaveSignal(aWidget, aDragContext, aTime, aData);
}

void 
nsWindow::OnDragLeaveSignal       (GtkWidget *      aWidget,
                                   GdkDragContext   *aDragContext,
                                   guint            aTime,
                                   gpointer         aData)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::OnDragLeaveSignal\n");
#endif 

  sIsDraggingOutOf = PR_TRUE;

  
  ResetDragMotionTimer(0, 0, 0, 0, 0);

  
  
  
  mDragLeaveTimer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ASSERTION(mDragLeaveTimer, "Failed to create drag leave timer!");
  
  mDragLeaveTimer->InitWithFuncCallback(DragLeaveTimerCallback, this, 0,
                                        nsITimer::TYPE_ONE_SHOT);
}


gint
nsWindow::DragDropSignal   (GtkWidget        *aWidget,
                            GdkDragContext   *aDragContext,
                            gint             aX,
                            gint             aY,
                            guint            aTime,
                            void             *aData)
{
  nsWindow *window = (nsWindow *)aData;
  return window->OnDragDropSignal(aWidget, aDragContext, aX, aY, aTime, aData);
}

gint
nsWindow::OnDragDropSignal        (GtkWidget        *aWidget,
                                   GdkDragContext   *aDragContext,
                                   gint             aX,
                                   gint             aY,
                                   guint            aTime,
                                   void             *aData)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::OnDragDropSignal\n");
#endif 

  
  nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
  nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

  nscoord retx = 0;
  nscoord rety = 0;
  
  Window thisWindow = GDK_WINDOW_XWINDOW(aWidget->window);
  Window returnWindow = None;
  returnWindow = GetInnerMostWindow(thisWindow, thisWindow, aX, aY, 
                                    &retx, &rety, 0);

  nsWindow *innerMostWidget = NULL;
  innerMostWidget = GetnsWindowFromXWindow(returnWindow);

  
  dragSessionGTK->TargetSetLastContext(aWidget, aDragContext, aTime);

  if (!innerMostWidget)
    innerMostWidget = this;

  
  if (mLastDragMotionWindow) {
    
    if (mLastDragMotionWindow != innerMostWidget) {
      
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

  
  
  

  innerMostWidget->AddRef();

  nsMouseEvent event(PR_TRUE, NS_DRAGDROP_OVER, innerMostWidget,
                     nsMouseEvent::eReal);

  InitDragEvent(event);

  
  UpdateDragStatus(event, aDragContext, dragService);

  event.refPoint.x = retx;
  event.refPoint.y = rety;

  innerMostWidget->DispatchMouseEvent(event);

  InitDragEvent(event);

  event.message = NS_DRAGDROP_DROP;
  event.widget = innerMostWidget;
  event.refPoint.x = retx;
  event.refPoint.y = rety;

  innerMostWidget->DispatchMouseEvent(event);

  innerMostWidget->Release();

  

  gdk_drop_finish(aDragContext, TRUE, aTime);

  
  
  
  
  dragSessionGTK->TargetSetLastContext(0, 0, 0);

  
  innerMostWidget->OnDragLeave();
  
  mLastDragMotionWindow = 0;

  
  
  dragService->EndDragSession();

  return TRUE;
}



void
nsWindow::DragDataReceived (GtkWidget         *aWidget,
                            GdkDragContext    *aDragContext,
                            gint               aX,
                            gint               aY,
                            GtkSelectionData  *aSelectionData,
                            guint              aInfo,
                            guint32            aTime,
                            gpointer           aData)
{
  nsWindow *window = (nsWindow *)aData;
  window->OnDragDataReceived(aWidget, aDragContext, aX, aY,
                             aSelectionData, aInfo, aTime, aData);
}

void
nsWindow::OnDragDataReceived      (GtkWidget         *aWidget,
                                   GdkDragContext    *aDragContext,
                                   gint               aX,
                                   gint               aY,
                                   GtkSelectionData  *aSelectionData,
                                   guint              aInfo,
                                   guint32            aTime,
                                   gpointer           aData)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::OnDragDataReceived\n");
#endif 
  
  nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);
  nsCOMPtr<nsIDragSessionGTK> dragSessionGTK = do_QueryInterface(dragService);

  dragSessionGTK->TargetDataReceived(aWidget, aDragContext, aX, aY,
                                     aSelectionData, aInfo, aTime);
}
 
void
nsWindow::OnDragLeave(void)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::OnDragLeave\n");
#endif 

  nsMouseEvent event(PR_TRUE, NS_DRAGDROP_EXIT, this, nsMouseEvent::eReal);

  AddRef();

  DispatchMouseEvent(event);

  nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);

  if (dragService) {
    nsCOMPtr<nsIDragSession> currentDragSession;
    dragService->GetCurrentSession(getter_AddRefs(currentDragSession));

    if (currentDragSession) {
      nsCOMPtr<nsIDOMNode> sourceNode;
      currentDragSession->GetSourceNode(getter_AddRefs(sourceNode));

      if (!sourceNode) {
        
        
        
        
        dragService->EndDragSession();
      }
    }
  }

  Release();
}

void
nsWindow::OnDragEnter(nscoord aX, nscoord aY)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::OnDragEnter\n");
#endif 

  nsRefPtr<nsWindow> kungFuDeathGrip(this);

  nsCOMPtr<nsIDragService> dragService = do_GetService(kCDragServiceCID);

  if (dragService) {
    
    dragService->StartDragSession();
  }

  nsMouseEvent event(PR_TRUE, NS_DRAGDROP_ENTER, this, nsMouseEvent::eReal);

  event.refPoint.x = aX;
  event.refPoint.y = aY;

  DispatchMouseEvent(event);
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
#ifdef DEBUG_DND_EVENTS
      g_print("*** canceled motion timer\n");
#endif
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
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::FireDragMotionTimer\n");
#endif
  OnDragMotionSignal(mDragMotionWidget, mDragMotionContext,
                     mDragMotionX, mDragMotionY, mDragMotionTime, 
                     this);
}

void
nsWindow::FireDragLeaveTimer(void)
{
#ifdef DEBUG_DND_EVENTS
  g_print("nsWindow::FireDragLeaveTimer\n");
#endif
  mDragLeaveTimer = 0;

  
  if (mLastDragMotionWindow) {
    
    mLastDragMotionWindow->OnDragLeave();
    mLastDragMotionWindow = 0;
  }

}


guint
nsWindow::DragMotionTimerCallback(gpointer aClosure)
{
  nsWindow *window = NS_STATIC_CAST(nsWindow *, aClosure);
  window->FireDragMotionTimer();
  return FALSE;
}


void
nsWindow::DragLeaveTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsWindow *window = NS_STATIC_CAST(nsWindow *, aClosure);
  window->FireDragLeaveTimer();
}


gint
nsWindow::ClientEventSignal(GtkWidget* widget, GdkEventClient* event, void* data)
{
  static GdkAtom atom_rcfiles = GDK_NONE;
  if (!atom_rcfiles)
    atom_rcfiles = gdk_atom_intern("_GTK_READ_RCFILES", FALSE);

  if (event->message_type == atom_rcfiles) {
    nsWidget* targetWindow = (nsWidget*) data;
    targetWindow->ThemeChanged();
  }

  return FALSE;
}

void
nsWindow::ThemeChanged()
{
  Display     *display;
  Window       window;
  Window       root_return;
  Window       parent_return;
  Window      *children_return = NULL;
  unsigned int nchildren_return = 0;
  unsigned int i = 0;

  if (mSuperWin)
  {
    display = GDK_DISPLAY();
    window = GDK_WINDOW_XWINDOW(mSuperWin->bin_window);
    if (window && !((GdkWindowPrivate *)mSuperWin->bin_window)->destroyed)
    {
      
      XQueryTree(display, window, &root_return, &parent_return,
                 &children_return, &nchildren_return);
      
      for (i=0; i < nchildren_return; i++)
      {
        Window child_window = children_return[i];
        nsWindow *thisWindow = GetnsWindowFromXWindow(child_window);
        if (thisWindow)
        {
          thisWindow->ThemeChanged();
        }
      }

      
      if (children_return)
        XFree(children_return);
    }
  }

  DispatchStandardEvent(NS_THEMECHANGED);
  Invalidate(PR_FALSE);
}

ChildWindow::ChildWindow()
{
}

ChildWindow::~ChildWindow()
{
#ifdef NOISY_DESTROY
  IndentByDepth(stdout);
  printf("ChildWindow::~ChildWindow:%p\n", this);
#endif
}

#ifdef USE_XIM
nsresult nsWindow::KillICSpotTimer ()
{
   if(mICSpotTimer)
   {
     mICSpotTimer->Cancel();
     mICSpotTimer = nsnull;
   }
   return NS_OK;
}

nsresult nsWindow::PrimeICSpotTimer ()
{
   KillICSpotTimer();
   nsresult err;
   mICSpotTimer = do_CreateInstance("@mozilla.org/timer;1", &err);
   if (NS_FAILED(err))
     return err;
   mICSpotTimer->InitWithFuncCallback(ICSpotCallback, this, 1000,
                                      nsITimer::TYPE_ONE_SHOT);
   return NS_OK;
}

void nsWindow::ICSpotCallback(nsITimer * aTimer, void * aClosure)
{
   nsWindow *window= NS_REINTERPRET_CAST(nsWindow*, aClosure);
   if( ! window) return;
   nsresult res = NS_ERROR_FAILURE;

   nsIMEGtkIC *xic = window->IMEGetInputContext(PR_FALSE);
   if (xic) {
     res = window->UpdateICSpot(xic);
   }
   if(NS_SUCCEEDED(res))
   {
      window->PrimeICSpotTimer();
   }
}

nsresult nsWindow::UpdateICSpot(nsIMEGtkIC *aXIC)
{
  
  nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_QUERY, this);
  static gint oldx =0;
  static gint oldy =0;
  static gint oldw =0;
  static gint oldh =0;
  compEvent.theReply.mCursorPosition.x=-1;
  compEvent.theReply.mCursorPosition.y=-1;
  this->OnComposition(compEvent);
  
  if((compEvent.theReply.mCursorPosition.x < 0) &&
     (compEvent.theReply.mCursorPosition.y < 0))
    return NS_ERROR_FAILURE;

  
  
  
  if((oldw != mBounds.width) || (oldh != mBounds.height)) {
    GdkWindow *gdkWindow = (GdkWindow*)this->GetNativeData(NS_NATIVE_WINDOW);
    if (gdkWindow) {
      aXIC->SetPreeditArea(0, 0,
                           (int)((GdkWindowPrivate*)gdkWindow)->width,
                           (int)((GdkWindowPrivate*)gdkWindow)->height);
    }
    oldw = mBounds.width;
    oldh = mBounds.height;
  }

  if((compEvent.theReply.mCursorPosition.x != oldx)||
     (compEvent.theReply.mCursorPosition.y != oldy))
  {
    nsPoint spot;
    spot.x = compEvent.theReply.mCursorPosition.x;
    spot.y = compEvent.theReply.mCursorPosition.y + 
      compEvent.theReply.mCursorPosition.height;
    SetXICBaseFontSize(aXIC, compEvent.theReply.mCursorPosition.height - 1);
    SetXICSpotLocation(aXIC, spot);
    oldx = compEvent.theReply.mCursorPosition.x;
    oldy = compEvent.theReply.mCursorPosition.y;
  } 
  return NS_OK;
}

void
nsWindow::SetXICBaseFontSize(nsIMEGtkIC* aXIC, int height)
{
  if (height%2) {
    height-=1;
  }
  if (height<2) return;
  if (height == mXICFontSize) return;
  if (gPreeditFontset) {
    gdk_font_unref(gPreeditFontset);
  }
  char *xlfdbase = PR_smprintf(XIC_FONTSET, height, height, height);
  gPreeditFontset = gdk_fontset_load(xlfdbase);
  if (gPreeditFontset) {
    aXIC->SetPreeditFont(gPreeditFontset);
  }
  mXICFontSize = height;
  PR_smprintf_free(xlfdbase);
}

void
nsWindow::SetXICSpotLocation(nsIMEGtkIC* aXIC, nsPoint aPoint)
{
  if (gPreeditFontset) {
    unsigned long x, y;
    x = aPoint.x, y = aPoint.y;
    y -= gPreeditFontset->descent;
    aXIC->SetPreeditSpotLocation(x, y);
  }
}

void
nsWindow::ime_preedit_start() {
}

void
nsWindow::ime_preedit_draw(nsIMEGtkIC *aXIC) {
  IMEComposeStart(nsnull);
  nsIMEPreedit *preedit = aXIC->GetPreedit();
  IMEComposeText(nsnull,
                 preedit->GetPreeditString(),
                 preedit->GetPreeditLength(),
                 preedit->GetPreeditFeedback());
  if (aXIC->IsPreeditComposing() == PR_FALSE) {
    IMEComposeEnd(nsnull);
  }
}

void
nsWindow::ime_preedit_done() {
#ifdef _AIX
  IMEComposeStart(nsnull);
  IMEComposeText(nsnull, nsnull, 0, nsnull);
#endif
  IMEComposeEnd(nsnull);
}

void
nsWindow::IMEUnsetFocusWindow()
{
  KillICSpotTimer();
}

void
nsWindow::IMESetFocusWindow()
{
  
  IMEGetShellWindow();

  nsIMEGtkIC *xic = IMEGetInputContext(PR_TRUE);

  if (xic) {
    if (xic->IsPreeditComposing() == PR_FALSE) {
      IMEComposeEnd(nsnull);
    }
    xic->SetFocusWindow(this);
    if (xic->mInputStyle & GDK_IM_PREEDIT_POSITION) {
      UpdateICSpot(xic);
      PrimeICSpotTimer();
    }
  }
}

void
nsWindow::IMEBeingActivate(PRBool aActive)
{
  
  if (mIMEShellWindow) {
    mIMEShellWindow->mIMEIsBeingActivate = aActive;
  } else {
    NS_ASSERTION(0, "mIMEShellWindow should exist");
  }
}

void
nsWindow::IMEGetShellWindow(void)
{
  if (!mIMEShellWindow) {
    nsWindow *mozAreaWindow = nsnull;
    GtkWidget *top_mozarea = GetOwningWidget();
    if (top_mozarea) {
      mozAreaWindow = NS_STATIC_CAST(nsWindow *,
                    gtk_object_get_data(GTK_OBJECT(top_mozarea), "nsWindow"));
    }
    mIMEShellWindow = mozAreaWindow;
    NS_ASSERTION(mIMEShellWindow, "IMEGetShellWindow() fails");
  }
}

nsIMEGtkIC*
nsWindow::IMEGetInputContext(PRBool aCreate)
{
  if (!mIMEShellWindow) {
    return nsnull;
  }

  nsXICLookupEntry* entry =
    NS_STATIC_CAST(nsXICLookupEntry *,
                   PL_DHashTableOperate(&gXICLookupTable, mIMEShellWindow,
                                        aCreate ? PL_DHASH_ADD
                                                : PL_DHASH_LOOKUP));

  if (!entry) {
    return nsnull;
  }
  if (PL_DHASH_ENTRY_IS_BUSY(entry) && entry->mXIC) {
    return entry->mXIC;
  }

  
  if (aCreate) {
    
    char *xlfdbase = PR_smprintf(XIC_FONTSET, mXICFontSize, mXICFontSize,
                                 mXICFontSize);
    if (xlfdbase) {
      if (gPreeditFontset == nsnull) {
        gPreeditFontset = gdk_fontset_load(xlfdbase);
      }
      if (gStatusFontset == nsnull) {
        gStatusFontset = gdk_fontset_load(xlfdbase);
      }
      PR_smprintf_free(xlfdbase);
      nsIMEGtkIC *xic = nsnull;
      if (gPreeditFontset && gStatusFontset) {
        xic = nsIMEGtkIC::GetXIC(mIMEShellWindow, gPreeditFontset,
                                 gStatusFontset);
        if (xic) {
          xic->SetPreeditSpotLocation(0, 14);
          entry->mShellWindow = mIMEShellWindow;
          entry->mXIC = xic;
          mIMEShellWindow->mIMEShellWindow = mIMEShellWindow;
          return xic;
        }
      }
    }

    
    PL_DHashTableRawRemove(&gXICLookupTable, entry);
  }

  return nsnull;
}

void
nsWindow::IMEDestroyIC()
{
  
  
  nsIMEGtkIC *xic = IMEGetInputContext(PR_FALSE);

  
  
  if (!xic) {
    return;
  }

  
  if (xic->mInputStyle & GDK_IM_STATUS_CALLBACKS) {
    xic->ResetStatusWindow(this);
  }

  if (mIMEShellWindow == this) {
    
    
    PL_DHashTableOperate(&gXICLookupTable, mIMEShellWindow, PL_DHASH_REMOVE);
    delete xic;
  } else {
    

    nsWindow *gwin = xic->GetGlobalFocusWindow();
    nsWindow *fwin = xic->GetFocusWindow();

    
    
    
    if (fwin && fwin == this) {
      xic->SetFocusWindow(mIMEShellWindow);
      xic->UnsetFocusWindow();

      
      
      
      if (gwin && gwin != this && sFocusWindow == gwin) {
        nsIMEGtkIC *focused_xic = gwin->IMEGetInputContext(PR_FALSE);
        if (focused_xic) {
          focused_xic->SetFocusWindow(gwin);
        }
      }
    }
  }
}
#endif 

void
nsWindow::IMEComposeStart(guint aTime)
{
#ifdef USE_XIM
  if (mIMECallComposeStart == PR_TRUE) {
    return;
  }
#endif 
  nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_START, this);
  compEvent.time = aTime;

  OnComposition(compEvent);

#ifdef USE_XIM
  mIMECallComposeStart = PR_TRUE;
  mIMECallComposeEnd = PR_FALSE;
#endif 
}

void
nsWindow::IMECommitEvent(GdkEventKey *aEvent) {
  PRInt32 srcLen = aEvent->length;

  if (srcLen && aEvent->string && aEvent->string[0] &&
      nsGtkIMEHelper::GetSingleton()) {

    PRInt32 uniCharSize;
    uniCharSize = nsGtkIMEHelper::GetSingleton()->MultiByteToUnicode(
                                aEvent->string,
                                srcLen,
                                &(mIMECompositionUniString),
                                &(mIMECompositionUniStringSize));

    if (uniCharSize) {
#ifdef USE_XIM
      nsIMEGtkIC *xic = IMEGetInputContext(PR_FALSE);
      mIMECompositionUniString[uniCharSize] = 0;
      if(sFocusWindow == 0 && xic != 0) {
        
        
        
        nsWindow *window = xic->GetFocusWindow();
        if (window) {
          window->IMEComposeStart(aEvent->time);
          window->IMEComposeText(aEvent,
                   mIMECompositionUniString,
                   uniCharSize,
                   nsnull);
          window->IMEComposeEnd(aEvent->time);
        }
      } else
#endif 
      {
        IMEComposeStart(aEvent->time);
        IMEComposeText(aEvent,
                   mIMECompositionUniString,
                   uniCharSize,
                   nsnull);
        IMEComposeEnd(aEvent->time);
      }
    }
  }

#ifdef USE_XIM
  nsIMEGtkIC *xic = IMEGetInputContext(PR_FALSE);
  if (xic) {
    if (xic->mInputStyle & GDK_IM_PREEDIT_POSITION) {
      nsWindow *window = xic->GetFocusWindow();
      if (window) {
        window->UpdateICSpot(xic);
        window->PrimeICSpotTimer();
      }
    }
  }
#endif 
}

void
nsWindow::IMEComposeText(GdkEventKey *aEvent,
                         const PRUnichar *aText, const PRInt32 aLen,
                         const char *aFeedback) {
  nsTextEvent textEvent(PR_TRUE, NS_TEXT_TEXT, this);
  if (aEvent) {
    textEvent.isShift = (aEvent->state & GDK_SHIFT_MASK) ? PR_TRUE : PR_FALSE;
    textEvent.isControl = (aEvent->state & GDK_CONTROL_MASK) ? PR_TRUE : PR_FALSE;
    textEvent.isAlt = (aEvent->state & GDK_MOD1_MASK) ? PR_TRUE : PR_FALSE;
    
    textEvent.isMeta = PR_FALSE; 
    textEvent.time = aEvent->time;
  }

  if (aLen != 0) {
    textEvent.theText = (PRUnichar*)aText;
#ifdef USE_XIM
    if (aFeedback) {
      nsIMEPreedit::IMSetTextRange(aLen,
                                   aFeedback,
                                   &(textEvent.rangeCount),
                                   &(textEvent.rangeArray));
    }
#endif 
  }
  OnText(textEvent);
#ifdef USE_XIM
  if (textEvent.rangeArray) {
    delete[] textEvent.rangeArray;
  }
#endif 
}

void
nsWindow::IMEComposeEnd(guint aTime)
{
#ifdef USE_XIM
  if (mIMECallComposeEnd == PR_TRUE) {
    return;
  }
#endif 

  nsCompositionEvent compEvent(PR_TRUE, NS_COMPOSITION_END, this);
  compEvent.time = aTime;
  OnComposition(compEvent);

#ifdef USE_XIM
  mIMECallComposeStart = PR_FALSE;
  mIMECallComposeEnd = PR_TRUE;
#endif 
}

NS_IMETHODIMP nsWindow::ResetInputState()
{
#ifdef USE_XIM
  nsIMEGtkIC *xic = IMEGetInputContext(PR_FALSE);
  if (xic) {
    
    
    if (mIMEShellWindow->mIMEIsBeingActivate == PR_TRUE) {
      return NS_OK;
    }

    
    
    if (mHasFocus == PR_FALSE) {
      return NS_OK;
    }

    
    if(xic->IsPreeditComposing() == PR_FALSE) {
      IMEComposeEnd(nsnull);
      return NS_OK;
    }

    
    PRInt32 uniCharSize = 
      xic->ResetIC(&(mIMECompositionUniString),
                    &(mIMECompositionUniStringSize));

    if (uniCharSize == 0) {
      
      
      if (xic->mInputStyle & GDK_IM_PREEDIT_CALLBACKS) {
        IMEComposeStart(nsnull);
        IMEComposeText(nsnull, nsnull, 0, nsnull);
        IMEComposeEnd(nsnull);
      }
    } else {
      mIMECompositionUniString[uniCharSize] = 0;
      IMEComposeStart(nsnull);
      IMEComposeText(nsnull,
                   mIMECompositionUniString,
                   uniCharSize,
                   nsnull);
      IMEComposeEnd(nsnull);
    }
    if (xic->mInputStyle & GDK_IM_PREEDIT_POSITION) {
      UpdateICSpot(xic);
    }
  }
#endif 
  return NS_OK;
}

static void
gdk_wmspec_change_state (gboolean   add,
                         GdkWindow *window,
                         GdkAtom    state1,
                         GdkAtom    state2)
{
  XEvent xev;

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.window = GDK_WINDOW_XWINDOW(window);
  xev.xclient.message_type = gdk_atom_intern("_NET_WM_STATE", FALSE);
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
  xev.xclient.data.l[1] = state1;
  xev.xclient.data.l[2] = state2;

  XSendEvent(gdk_display, GDK_ROOT_WINDOW(), False,
             SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}

#ifndef MOZ_XUL
void nsWindow::ApplyTransparencyBitmap() {}
void nsWindow::ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight) {
}
#else
NS_IMETHODIMP nsWindow::SetWindowTranslucency(PRBool aTranslucent) {
  if (!mMozArea)
    return GetOwningWindow()->SetWindowTranslucency(aTranslucent);

  if (!mShell) {
    
    NS_WARNING("Trying to use transparent chrome in an embedded context");
    return NS_ERROR_FAILURE;
  }

  if (mIsTranslucent == aTranslucent)
    return NS_OK;

  if (!aTranslucent) {
    if (mTransparencyBitmap) {
      delete[] mTransparencyBitmap;
      mTransparencyBitmap = nsnull;
      gtk_widget_reset_shapes(mShell);
    }
  } 
    

  mIsTranslucent = aTranslucent;
  return NS_OK;
}

NS_IMETHODIMP nsWindow::GetWindowTranslucency(PRBool& aTranslucent) {
  if (!mMozArea)
    return GetOwningWindow()->GetWindowTranslucency(aTranslucent);

  aTranslucent = mIsTranslucent;
  return NS_OK;
}

static gchar* CreateDefaultTransparencyBitmap(PRInt32 aWidth, PRInt32 aHeight) {
  PRInt32 size = ((aWidth+7)/8)*aHeight;
  gchar* bits = new gchar[size];
  if (bits) {
    memset(bits, 255, size);
  }
  return bits;
}

void nsWindow::ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight) {
  if (!mTransparencyBitmap)
    return;

  gchar* newBits = CreateDefaultTransparencyBitmap(aNewWidth, aNewHeight);
  if (!newBits) {
    delete[] mTransparencyBitmap;
    mTransparencyBitmap = nsnull;
    return;
  }

  
  PRInt32 copyWidth = PR_MIN(aNewWidth, mBounds.width);
  PRInt32 copyHeight = PR_MIN(aNewHeight, mBounds.height);
  PRInt32 oldRowBytes = (mBounds.width+7)/8;
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
}

static PRBool ChangedMaskBits(gchar* aMaskBits, PRInt32 aMaskWidth, PRInt32 aMaskHeight,
                              const nsRect& aRect, PRUint8* aAlphas) {
  PRInt32 x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
  PRInt32 maskBytesPerRow = (aMaskWidth + 7)/8;
  for (y = aRect.y; y < yMax; y++) {
    gchar* maskBytes = aMaskBits + y*maskBytesPerRow;
    for (x = aRect.x; x < xMax; x++) {
      PRBool newBit = *aAlphas > 0;
      aAlphas++;

      gchar maskByte = maskBytes[x >> 3];
      PRBool maskBit = (maskByte & (1 << (x & 7))) != 0;

      if (maskBit != newBit) {
        return PR_TRUE;
      }
    }
  }

  return PR_FALSE;
}

static void UpdateMaskBits(gchar* aMaskBits, PRInt32 aMaskWidth, PRInt32 aMaskHeight,
                           const nsRect& aRect, PRUint8* aAlphas) {
  PRInt32 x, y, xMax = aRect.XMost(), yMax = aRect.YMost();
  PRInt32 maskBytesPerRow = (aMaskWidth + 7)/8;
  for (y = aRect.y; y < yMax; y++) {
    gchar* maskBytes = aMaskBits + y*maskBytesPerRow;
    for (x = aRect.x; x < xMax; x++) {
      PRBool newBit = *aAlphas > 0;
      aAlphas++;

      gchar mask = 1 << (x & 7);
      gchar maskByte = maskBytes[x >> 3];
      
      maskBytes[x >> 3] = (maskByte & ~mask) | (-newBit & mask);
    }
  }
}

void nsWindow::ApplyTransparencyBitmap() {
  if (!mTransparencyBitmap) {
    mTransparencyBitmap = CreateDefaultTransparencyBitmap(mBounds.width, mBounds.height);
    if (!mTransparencyBitmap)
      return;
  }

  gtk_widget_reset_shapes(mShell);
  GdkBitmap* maskBitmap = gdk_bitmap_create_from_data(mShell->window,
                                                      mTransparencyBitmap,
                                                      mBounds.width, mBounds.height);
  if (!maskBitmap)
    return;

  gtk_widget_shape_combine_mask(mShell, maskBitmap, 0, 0);
  gdk_bitmap_unref(maskBitmap);
}

NS_IMETHODIMP nsWindow::UpdateTranslucentWindowAlpha(const nsRect& aRect,
                                                     PRUint8* aAlphas)
{
  if (!mMozArea)
    return GetOwningWindow()->UpdateTranslucentWindowAlpha(aRect, aAlphas);

  NS_ASSERTION(mIsTranslucent, "Window is not transparent");

  if (!mTransparencyBitmap) {
    mTransparencyBitmap = CreateDefaultTransparencyBitmap(mBounds.width, mBounds.height);
    if (!mTransparencyBitmap)
      return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(aRect.x >= 0 && aRect.y >= 0
               && aRect.XMost() <= mBounds.width && aRect.YMost() <= mBounds.height,
               "Rect is out of window bounds");

  if (!ChangedMaskBits(mTransparencyBitmap, mBounds.width, mBounds.height, aRect, aAlphas))
    
    
    return NS_OK;

  UpdateMaskBits(mTransparencyBitmap, mBounds.width, mBounds.height, aRect, aAlphas);

  if (mShown) {
    ApplyTransparencyBitmap();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::HideWindowChrome(PRBool aShouldHide)
{
  if (!mMozArea)
    return GetOwningWindow()->HideWindowChrome(aShouldHide);

  if (!mShell) {
    
    NS_WARNING("Trying to hide window decorations in an embedded context");
    return NS_ERROR_FAILURE;
  }

  
  
  

  if (mShown)
    gdk_window_hide(mShell->window);

  gint wmd;
  if (aShouldHide)
    wmd = 0;
  else
    wmd = ConvertBorderStyles(mBorderStyle);

  gdk_window_set_decorations(mShell->window, (GdkWMDecoration) wmd);

  if (mShown) 
    gdk_window_show(mShell->window);
  
  

  
  
  
  
  
  XSync(GDK_DISPLAY(), False);

  return NS_OK;
}

NS_IMETHODIMP
nsWindow::MakeFullScreen(PRBool aFullScreen)
{
  if (!mMozArea)
    return GetOwningWindow()->MakeFullScreen(aFullScreen);

  if (!mShell) {
    
    NS_WARNING("Trying to go fullscreen in an embedded context");
    return NS_ERROR_FAILURE;
  }

  gdk_wmspec_change_state(aFullScreen, mShell->window,
                          gdk_atom_intern("_NET_WM_STATE_FULLSCREEN", False),
                          GDK_NONE);
  return NS_OK;
}
#endif

void PR_CALLBACK
nsWindow::ClearIconEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr)
{
  IconEntry* entry = NS_STATIC_CAST(IconEntry*, aHdr);
  if (entry->w_pixmap) {
    gdk_pixmap_unref(entry->w_pixmap);
    gdk_bitmap_unref(entry->w_mask);
  }
  if (entry->w_minipixmap) {
    gdk_pixmap_unref(entry->w_minipixmap);
    gdk_bitmap_unref(entry->w_minimask);
  }
  if (entry->string)
    free((void*) entry->string);
  PL_DHashClearEntryStub(aTable, aHdr);
}


int
is_parent_ungrab_enter(GdkEventCrossing *event)
{
  return (GDK_CROSSING_UNGRAB == event->mode) &&
    ((GDK_NOTIFY_ANCESTOR == event->detail) ||
     (GDK_NOTIFY_VIRTUAL == event->detail));
}


int
is_parent_grab_leave(GdkEventCrossing *event)
{
  return (GDK_CROSSING_GRAB == event->mode) &&
    ((GDK_NOTIFY_ANCESTOR == event->detail) ||
     (GDK_NOTIFY_VIRTUAL == event->detail));
}
