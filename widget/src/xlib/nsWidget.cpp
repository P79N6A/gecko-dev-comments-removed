












































#undef DEBUG_CURSORCACHE

#include "nsWidget.h"
#include "nsIServiceManager.h"
#include "nsAppShell.h"

#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include "nsXlibCursors.h"

#include "nsIEventListener.h"
#include "nsIMenuListener.h"
#include "nsIMouseListener.h"
#include "nsIRollupListener.h"
#include "nsGfxCIID.h"
#include "nsIMenuRollup.h"
#include "nsIRenderingContext.h"
#include "nsToolkit.h"

#include "xlibrgb.h"

static NS_DEFINE_CID(kRegionCID, NS_REGION_CID);

PRLogModuleInfo *XlibWidgetsLM   = PR_NewLogModule("XlibWidgets");
PRLogModuleInfo *XlibScrollingLM = PR_NewLogModule("XlibScrolling");


nsHashtable *nsWidget::gsWindowList = nsnull; 


Cursor nsWidget::gsXlibCursorCache[eCursorCount];


PRBool nsWidget::WMProtocolsInitialized = PR_FALSE;
Atom   nsWidget::WMDeleteWindow = 0;
Atom   nsWidget::WMTakeFocus    = 0;
Atom   nsWidget::WMSaveYourself = 0;


Window nsWidget::mFocusWindow = 0;


nsCOMPtr<nsIRollupListener> nsWidget::gRollupListener;
nsWeakPtr          nsWidget::gRollupWidget;
PRBool             nsWidget::gRollupConsumeRollupEvent = PR_FALSE;

class nsWindowKey : public nsHashKey {
protected:
  Window mKey;

public:
  nsWindowKey(Window key) {
    mKey = key;
  }
  ~nsWindowKey(void) {
  }
  PRUint32 HashCode(void) const {
    return (PRUint32)mKey;
  }

  PRBool Equals(const nsHashKey *aKey) const {
    return (mKey == ((nsWindowKey *)aKey)->mKey);
  }

  nsHashKey *Clone(void) const {
    return new nsWindowKey(mKey);
  }
};

nsWidget::nsWidget() 
{
  mPreferredWidth = 0;
  mPreferredHeight = 0;

  mDisplay = 0;
  mScreen = 0;
  mVisual = 0;
  mDepth = 0;

  mBaseWindow = 0;
  mBackground = NS_RGB(192, 192, 192);
  mBorderRGB = NS_RGB(192, 192, 192);
  

  mXlibRgbHandle = nsAppShell::GetXlibRgbHandle();
  mBackgroundPixel = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, mBackground);
  mBackground = NS_RGB(192, 192, 192);
  mBorderPixel = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, mBorderRGB);
  mParentWidget = nsnull;
  mName.AssignLiteral("unnamed");
  mIsShown = PR_FALSE;
  mIsToplevel = PR_FALSE;
  mVisibility = VisibilityFullyObscured; 
  mWindowType = eWindowType_child;
  mBorderStyle = eBorderStyle_default;

  
  mIsDestroying = PR_FALSE;
  mOnDestroyCalled = PR_FALSE;
  mListenForResizes = PR_FALSE; 
  mMapped = PR_FALSE;


  mUpdateArea = do_CreateInstance(kRegionCID);
  if (mUpdateArea) {
    mUpdateArea->Init();
    mUpdateArea->SetTo(0, 0, 0, 0);
  }
}



nsWidget::~nsWidget()
{
  

  if (mBaseWindow)
    Destroy();
}




void
nsWidget::DestroyNativeChildren(Display *aDisplay, Window aWindow)
{
  Window       root_return;
  Window       parent_return;
  Window      *children_return = nsnull;
  unsigned int nchildren_return = 0;
  unsigned int i = 0;
  
  XQueryTree(aDisplay, aWindow, &root_return, &parent_return,
             &children_return, &nchildren_return);
  
  for (i=0; i < nchildren_return; i++) {
    nsWidget *thisWidget = GetWidgetForWindow(children_return[i]);
    if (thisWidget) {
      thisWidget->Destroy();
    }
  }      

  
  if (children_return)
    XFree(children_return);
}

void
nsWidget::DestroyNative()
{
  NS_ASSERTION(mBaseWindow, "no native window");

  
  
  
  DestroyNativeChildren(mDisplay, mBaseWindow);

  XDestroyWindow(mDisplay, mBaseWindow);
  DeleteWindowCallback(mBaseWindow);
}


void * nsWidget::CheckParent(long ThisWindow)
{
  return (void*)-1;
}

NS_IMETHODIMP nsWidget::Create(nsIWidget *aParent,
                               const nsRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell,
                               nsIToolkit *aToolkit,
                               nsWidgetInitData *aInitData)
{
  
  
  

  return StandardWidgetCreate(aParent, aRect, aHandleEventFunction,
                              aContext, aAppShell, aToolkit, aInitData,
                              nsnull);
}

NS_IMETHODIMP nsWidget::Create(nsNativeWidget aParent,
                               const nsRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell,
                               nsIToolkit *aToolkit,
                               nsWidgetInitData *aInitData)
{
  return(StandardWidgetCreate(nsnull, aRect, aHandleEventFunction,
                              aContext, aAppShell, aToolkit, aInitData,
                              aParent));
}

nsresult
nsWidget::StandardWidgetCreate(nsIWidget *aParent,
                               const nsRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell,
                               nsIToolkit *aToolkit,
                               nsWidgetInitData *aInitData,
                               nsNativeWidget aNativeParent)
{
  unsigned long attr_mask;
  XSetWindowAttributes attr;
  Window parent=nsnull;

  NS_ASSERTION(!mBaseWindow, "already initialized");
  if (mBaseWindow) return NS_ERROR_ALREADY_INITIALIZED;

  mDisplay = xxlib_rgb_get_display(mXlibRgbHandle);
  mScreen = xxlib_rgb_get_screen(mXlibRgbHandle);
  mVisual = xxlib_rgb_get_visual(mXlibRgbHandle);
  mDepth = xxlib_rgb_get_depth(mXlibRgbHandle);

  mParentWidget = aParent;

  NS_IF_ADDREF(mParentWidget);

  mBounds = aRect;

  if (mBounds.width <= 0)
    mBounds.width = 1;
  if (mBounds.height <= 0)
    mBounds.height = 1;

  BaseCreate(mParentWidget, mBounds, aHandleEventFunction, aContext,
             aAppShell, aToolkit, aInitData);

  if (aNativeParent) {
    parent = (Window)aNativeParent;
    mListenForResizes = PR_TRUE;
  } else if (aParent) {
    parent = (Window)aParent->GetNativeData(NS_NATIVE_WINDOW);
  } else {
    parent = XRootWindowOfScreen(mScreen);
  }

  if (nsnull != aInitData) {
    mWindowType = aInitData->mWindowType;
  }

  mParentWindow = parent;

  attr.bit_gravity       = NorthWestGravity;
  attr.event_mask        = GetEventMask();
  attr.colormap          = xxlib_rgb_get_cmap(mXlibRgbHandle);
  attr.background_pixel  = mBackgroundPixel;
  attr.border_pixel      = mBorderPixel;
  attr_mask = CWBitGravity | CWEventMask | CWBorderPixel | CWBackPixel;

  if (attr.colormap)
    attr_mask |= CWColormap;

  switch (mWindowType) {
  case eWindowType_dialog:
    mIsToplevel = PR_TRUE;
    parent = XRootWindowOfScreen(mScreen);
    mBaseWindow = XCreateWindow(mDisplay, parent, mBounds.x, mBounds.y,
                                mBounds.width, mBounds.height, 0,
                                mDepth, InputOutput, mVisual,
                                attr_mask, &attr);
    XSetWindowBackgroundPixmap(mDisplay, mBaseWindow, None);
    AddWindowCallback(mBaseWindow, this);
    SetUpWMHints();
    XSetTransientForHint(mDisplay, mBaseWindow, mParentWindow);
    break;

  case eWindowType_popup:
    mIsToplevel = PR_TRUE;
    attr_mask |= CWOverrideRedirect | CWSaveUnder;
    attr.save_under = True;
    attr.override_redirect = True;
    parent = XRootWindowOfScreen(mScreen);
    mBaseWindow = XCreateWindow(mDisplay, parent,
                                mBounds.x, mBounds.y,
                                mBounds.width, mBounds.height, 0,
                                mDepth, InputOutput, mVisual,
                                attr_mask, &attr);
    XSetWindowBackgroundPixmap(mDisplay, mBaseWindow, None);
    AddWindowCallback(mBaseWindow, this);
    SetUpWMHints();
    XSetTransientForHint(mDisplay, mBaseWindow, mParentWindow);
    break;

  case eWindowType_toplevel:
  case eWindowType_invisible:
    mIsToplevel = PR_TRUE;
    parent = XRootWindowOfScreen(mScreen);
    mBaseWindow = XCreateWindow(mDisplay, parent, mBounds.x, mBounds.y,
                                mBounds.width, mBounds.height, 0,
                                mDepth, InputOutput, mVisual,
                                attr_mask, &attr);
    XSetWindowBackgroundPixmap(mDisplay, mBaseWindow, None);
    AddWindowCallback(mBaseWindow, this);
    SetUpWMHints();
    break;

  case eWindowType_child:
    mIsToplevel = PR_FALSE;
    
    CreateNative(parent, mBounds);
    break;

  default:
    break;
  }
  
  return NS_OK;
}



NS_IMETHODIMP nsWidget::Destroy()
{

  
  if (mIsDestroying)
    return NS_OK;

  mIsDestroying = PR_TRUE;

  nsBaseWidget::Destroy();
  NS_IF_RELEASE(mParentWidget); 
  

  if (mBaseWindow) {
    DestroyNative();
    
    if (PR_FALSE == mOnDestroyCalled)
      OnDestroy();
    mBaseWindow = nsnull;
    mEventCallback = nsnull;
  }

  return NS_OK;

}

NS_IMETHODIMP nsWidget::ConstrainPosition(PRBool aAllowSlop,
                                          PRInt32 *aX, PRInt32 *aY)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Move(PRInt32 aX, PRInt32 aY)
{
  
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Move(x, y)\n"));
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Moving window 0x%lx to %d, %d\n", mBaseWindow, aX, aY));

  if((aX == mBounds.x) && (aY == mBounds.y) && !mIsToplevel) {
    
    return NS_OK;
  }


  mBounds.x = aX;
  mBounds.y = aY;

  if (mWindowType == eWindowType_popup) {
    nsRect aRect, transRect;
    PRInt32 screenWidth = WidthOfScreen(mScreen);
    PRInt32 screenHeight = HeightOfScreen(mScreen);

    if (aX >= screenWidth)
      aX = screenWidth - mBounds.width;
    if (aY >= screenHeight)
      aY = screenHeight - mBounds.height;

    aRect.x = aX;
    aRect.y = aY;

    if (mParentWidget) {
      mParentWidget->WidgetToScreen(aRect, transRect);
    } else if (mParentWindow) {
      Window child;
      XTranslateCoordinates(mDisplay, mParentWindow,
                            XRootWindowOfScreen(mScreen),
                            aX, aY, &transRect.x, &transRect.y,
                            &child);
    }
    aX = transRect.x;
    aY = transRect.y;
  }

  mRequestedSize.x = aX;
  mRequestedSize.y = aY;
  if (mParentWidget) {
    ((nsWidget*)mParentWidget)->WidgetMove(this);
  } else {
    
    if (mDisplay)
      XMoveWindow(mDisplay, mBaseWindow, aX, aY);
  }
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Resize(PRInt32 aWidth,
                               PRInt32 aHeight,
                               PRBool   aRepaint)
{
  
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Resize(width, height)\n"));

  if (aWidth <= 0) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("*** width is %d, fixing.\n", aWidth));
    aWidth = 1;

  }
  if (aHeight <= 0) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("*** height is %d, fixing.\n", aHeight));
    aHeight = 1;
  }

  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Resizing window 0x%lx to %d, %d\n", mBaseWindow, aWidth, aHeight));
  mRequestedSize.width = mBounds.width = aWidth;
  mRequestedSize.height = mBounds.height = aHeight;

  if (mParentWidget) {
    ((nsWidget *)mParentWidget)->WidgetResize(this);
  } else {
     
    if (mDisplay)
      XResizeWindow(mDisplay, mBaseWindow, aWidth, aHeight);
  }

  return NS_OK;
}

NS_IMETHODIMP nsWidget::Resize(PRInt32 aX,
                               PRInt32 aY,
                               PRInt32 aWidth,
                               PRInt32 aHeight,
                               PRBool   aRepaint)
{
  
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Resize(x, y, width, height)\n"));

  if (aWidth <= 0) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("*** width is %d, fixing.\n", aWidth));
    aWidth = 1;
  }
  if (aHeight <= 0) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("*** height is %d, fixing.\n", aHeight));
    aHeight = 1;
  }
  if (aX < 0) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("*** x is %d, fixing.\n", aX));
    aX = 0;
  }
  if (aY < 0) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("*** y is %d, fixing.\n", aY));
    aY = 0;
  }
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG,
         ("Resizing window 0x%lx to %d, %d\n", mBaseWindow, aWidth, aHeight));
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, 
         ("Moving window 0x%lx to %d, %d\n", mBaseWindow, aX, aY));
  mRequestedSize.x = aX;
  mRequestedSize.y = aY;
  mRequestedSize.width = mBounds.width = aWidth;
  mRequestedSize.height = mBounds.height = aHeight;
  if (mParentWidget) {
    ((nsWidget *)mParentWidget)->WidgetMoveResize(this);
  } else {
    XMoveResizeWindow(mDisplay, mBaseWindow, aX, aY, aWidth, aHeight);
  }
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Enable(PRBool aState)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::IsEnabled(PRBool *aState)
{
  NS_ENSURE_ARG_POINTER(aState);
  *aState = PR_TRUE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsWidget::SetFocus(PRBool aRaise)
{

  if (mBaseWindow) {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::SetFocus() setting focus to 0x%lx\n", mBaseWindow));
    mFocusWindow = mBaseWindow;
  }
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetName(const char * aName)
{
  mName.AssignWithConversion(aName);

  return NS_OK;
}

Window
nsWidget::GetFocusWindow(void)
{
  return mFocusWindow;
}

NS_IMETHODIMP nsWidget::Invalidate(PRBool aIsSynchronous)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Invalidate(sync)\n"));
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Invalidate(rect, sync)\n"));
  return NS_OK;
}

nsIFontMetrics* nsWidget::GetFont(void)
{
  return nsnull;
}

NS_IMETHODIMP nsWidget::GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetMenuBar(nsIMenuBar * aMenuBar)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::ShowMenuBar(PRBool aShow)
{
  return NS_OK;
}

void * nsWidget::GetNativeData(PRUint32 aDataType)
{
  switch (aDataType) {
  case NS_NATIVE_WIDGET:
  case NS_NATIVE_WINDOW:
  case NS_NATIVE_PLUGIN_PORT:
    return (void *)mBaseWindow;
    break;
  case NS_NATIVE_DISPLAY:
    return (void *)mDisplay;
    break;
  case NS_NATIVE_GRAPHIC:
    NS_ASSERTION(nsnull != mToolkit, "NULL toolkit, unable to get a GC");
    return (void *)NS_STATIC_CAST(nsToolkit*,mToolkit)->GetSharedGC();
    break;
  default:
    fprintf(stderr, "nsWidget::GetNativeData(%d) called with crap value.\n",
            aDataType);
    return nsnull;
    break;
  }
}

NS_IMETHODIMP nsWidget::SetTooltips(PRUint32 aNumberOfTips,
                                    nsRect* aTooltipAreas[])
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::UpdateTooltips(nsRect* aNewTips[])
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::RemoveTooltips()
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetFont(const nsFont &aFont)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::BeginResizingChildren(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::EndResizingChildren(void)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetColorMap(nsColorMap *aColorMap)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Show(PRBool bState)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Show()\n"));
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("state is %d\n", bState));

  if (bState) {
        Map();
  } else {
      Unmap();
    }
  return NS_OK;
}

NS_IMETHODIMP nsWidget::IsVisible(PRBool &aState)
{
  if (mVisibility != VisibilityFullyObscured) {
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::IsVisible: yes\n"));
    aState = PR_TRUE;
  }
  else {
    PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::IsVisible: no\n"));
    aState = PR_FALSE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Update()
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::Update()\n"));
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetBackgroundColor(const nscolor &aColor)
{
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("nsWidget::SetBackgroundColor()\n"));

  nsBaseWidget::SetBackgroundColor(aColor);
  mBackgroundPixel = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, mBackground);
  
  XSetWindowBackground(mDisplay, mBaseWindow, mBackgroundPixel);
  return NS_OK;
}

NS_IMETHODIMP nsWidget::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED1(nsWidget, nsBaseWidget, nsISupportsWeakReference)

NS_IMETHODIMP nsWidget::WidgetToScreen(const nsRect& aOldRect,
                                       nsRect& aNewRect)
{
  Window child;
  XTranslateCoordinates(mDisplay,
                        mBaseWindow,
                        XRootWindowOfScreen(mScreen),
                        aOldRect.x, aOldRect.y,
                        &aNewRect.x, &aNewRect.y,
                        &child);
  return NS_OK;
}

NS_IMETHODIMP nsWidget::ScreenToWidget(const nsRect& aOldRect,
                                       nsRect& aNewRect)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::SetCursor(nsCursor aCursor)
{

  if (!mBaseWindow)
    return NS_ERROR_FAILURE;

  
  if (!mMapped)
    return NS_OK;
  
  
  if (aCursor != mCursor) {
    Cursor newCursor = None;

    newCursor = XlibCreateCursor(aCursor);

    if (None != newCursor) {
      mCursor = aCursor;
      XDefineCursor(mDisplay, mBaseWindow, newCursor);
      XFlush(mDisplay);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsWidget::PreCreateWidget(nsWidgetInitData *aInitData)
{
  if (nsnull != aInitData) {
    SetWindowType(aInitData->mWindowType);
    SetBorderStyle(aInitData->mBorderStyle);
    
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsIWidget *nsWidget::GetParent(void)
{
  return mParentWidget;
}

void nsWidget::CreateNative(Window aParent, nsRect aRect)
{
  XSetWindowAttributes attr;
  unsigned long attr_mask;

  attr.bit_gravity       = NorthWestGravity;
  attr.event_mask        = GetEventMask();
  attr.colormap          = xxlib_rgb_get_cmap(mXlibRgbHandle);
  attr.background_pixel  = mBackgroundPixel;
  attr.border_pixel      = mBorderPixel;
  attr_mask = CWBitGravity | CWEventMask | CWBorderPixel | CWBackPixel;

  if (attr.colormap)
    attr_mask |= CWColormap;

  CreateNativeWindow(aParent, mBounds, attr, attr_mask);
}

 long
nsWidget::GetEventMask()
{
  long event_mask;

  event_mask = 
    ButtonMotionMask |
    Button1MotionMask |
    ButtonPressMask |
    ButtonReleaseMask |
    EnterWindowMask |
    ExposureMask |
    KeyPressMask |
    KeyReleaseMask |
    LeaveWindowMask |
    PointerMotionMask |
    StructureNotifyMask |
    VisibilityChangeMask |
    FocusChangeMask |
    OwnerGrabButtonMask;

  return event_mask;
}

void nsWidget::CreateNativeWindow(Window aParent, nsRect aRect,
                                  XSetWindowAttributes aAttr, unsigned long aMask)
{
  NS_ASSERTION(!mBaseWindow, "already initialized");
  if (mBaseWindow) return;

  mBaseWindow = XCreateWindow(mDisplay,
                              aParent,
                              aRect.x, aRect.y,
                              aRect.width, aRect.height,
                              0,                
                              mDepth,           
                              InputOutput,      
                              mVisual,          
                              aMask,
                              &aAttr);
  XSetWindowBackgroundPixmap(mDisplay, mBaseWindow, None);

  mRequestedSize.height = mBounds.height = aRect.height;
  mRequestedSize.width = mBounds.width = aRect.width;
  AddWindowCallback(mBaseWindow, this);
}

nsWidget *
nsWidget::GetWidgetForWindow(Window aWindow)
{
  if (gsWindowList == nsnull) {
    return nsnull;
  }
  nsWindowKey window_key(aWindow);
  nsWidget *retval = (nsWidget *)gsWindowList->Get(&window_key);
  return retval;
}

void
nsWidget::AddWindowCallback(Window aWindow, nsWidget *aWidget)
{
  
  if (gsWindowList == nsnull) {
    gsWindowList = new nsHashtable();
  }
  nsWindowKey window_key(aWindow);
  gsWindowList->Put(&window_key, aWidget);
}

void
nsWidget::DeleteWindowCallback(Window aWindow)
{
  nsWindowKey window_key(aWindow);
  gsWindowList->Remove(&window_key);

  if (gsWindowList->Count() == 0) {
    delete gsWindowList;
    gsWindowList = nsnull;

    
#ifdef DEBUG_CURSORCACHE
    printf("freeing cursor cache\n");
#endif
    for (int i = 0; i < eCursorCount; i++)
      if (gsXlibCursorCache[i])
        XFreeCursor(nsAppShell::mDisplay, gsXlibCursorCache[i]);
  }
}

#undef TRACE_PAINT
#undef TRACE_PAINT_FLASH

#ifdef TRACE_PAINT_FLASH
#include "nsXUtils.h" 
#endif

PRBool
nsWidget::OnPaint(nsPaintEvent &event)
{
  nsresult result = PR_FALSE;
  if (mEventCallback) {
    event.renderingContext = GetRenderingContext();
    if (event.renderingContext) {
      result = DispatchWindowEvent(event);
      NS_RELEASE(event.renderingContext);
    }
  }
  
#ifdef TRACE_PAINT
  static PRInt32 sPrintCount = 0;

  if (event.rect) 
  {
    printf("%4d nsWidget::OnPaint   (this=%p,name=%s,xid=%p,rect=%d,%d,%d,%d)\n", 
           sPrintCount++,
           (void *) this,
           (const char *) nsCAutoString(mName),
           (void *) mBaseWindow,
           event.rect->x, 
           event.rect->y,
           event.rect->width, 
           event.rect->height);
  }
  else 
  {
    printf("%4d nsWidget::OnPaint   (this=%p,name=%s,xid=%p,rect=none)\n", 
           sPrintCount++,
           (void *) this,
           (const char *) nsCAutoString(mName),
           (void *) mBaseWindow);
  }
#endif

#ifdef TRACE_PAINT_FLASH
    XRectangle ar;
    XRectangle * area = nsnull;

    if (event.rect)
    {
      ar.x = event.rect->x;
      ar.y = event.rect->y;

      ar.width = event.rect->width;
      ar.height = event.rect->height;

      area = &ar;
    }

    nsXUtils::XFlashWindow(mDisplay,mBaseWindow,1,100000,area);
#endif


  return result;
}

PRBool nsWidget::IsMouseInWindow(Window window, PRInt32 inMouseX, PRInt32 inMouseY)
{
  XWindowAttributes inWindowAttributes;

  
  if (!window)
    return PR_FALSE;

  
  if (XGetWindowAttributes(mDisplay, window, &inWindowAttributes) == 0) {
    fprintf(stderr, "Failed calling XGetWindowAttributes in nsWidget::IsMouseInWindow");
    return PR_FALSE;
  }

  
  

  
  int root_inMouse_x,
      root_inMouse_y;
  Window returnedChild;
  Window rootWindow;
  rootWindow = XRootWindow(mDisplay, XDefaultScreen(mDisplay));
  if (!XTranslateCoordinates(mDisplay, mBaseWindow, rootWindow,
    inMouseX, inMouseY,
    &root_inMouse_x, &root_inMouse_y, &returnedChild)){
    fprintf(stderr, "Could not get coordinates for origin coordinates for mouseclick\n");
    
    return PR_FALSE;
  }
  

  
  if (root_inMouse_x > inWindowAttributes.x &&
      root_inMouse_x < (inWindowAttributes.x + inWindowAttributes.width) &&
      root_inMouse_y > inWindowAttributes.y &&
      root_inMouse_y < (inWindowAttributes.y + inWindowAttributes.height)) {
#ifdef DEBUG_whoemeveraddedthatprintforginally
    
#endif
    return PR_TRUE;
  }
  
#ifdef DEBUG_whoemeveraddedthatprintforginally
  
#endif
  return PR_FALSE;
}







PRBool nsWidget::HandlePopup ( PRInt32 inMouseX, PRInt32 inMouseY )
{
  PRBool retVal = PR_FALSE;
  PRBool rollup = PR_FALSE;

  
  

  nsCOMPtr<nsIWidget> rollupWidget = do_QueryReferent(gRollupWidget);

  if (rollupWidget && gRollupListener) {
    Window currentPopup = (Window)rollupWidget->GetNativeData(NS_NATIVE_WINDOW);

    if (!IsMouseInWindow(currentPopup, inMouseX, inMouseY)) {
      rollup = PR_TRUE;
      nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(gRollupListener) );
      
      if ( menuRollup ) {
        nsCOMPtr<nsISupportsArray> widgetChain;
        menuRollup->GetSubmenuWidgetChain ( getter_AddRefs(widgetChain) );
        if ( widgetChain ) {
          PRUint32 count = 0;
          widgetChain->Count ( &count );
          for ( PRUint32 i = 0; i < count; ++i ) {
            nsCOMPtr<nsISupports> genericWidget;
            widgetChain->GetElementAt ( i, getter_AddRefs(genericWidget) );
            nsCOMPtr<nsIWidget> widget ( do_QueryInterface(genericWidget) );
            if ( widget ) {
              Window currWindow = (Window)widget->GetNativeData(NS_NATIVE_WINDOW);
              if ( IsMouseInWindow(currWindow, inMouseX, inMouseY) ) {
                rollup = PR_FALSE;
                break;
              }
            }
          } 
        }
      }
    }
  }

  if (rollup) {
    gRollupListener->Rollup();
    retVal = PR_TRUE;
  }
  return retVal;
}



void nsWidget::OnDestroy()
{
  mOnDestroyCalled = PR_TRUE;
  nsBaseWidget::OnDestroy();

  
  
  
  
  DispatchDestroyEvent();
}

PRBool nsWidget::OnDeleteWindow(void)
{
#ifdef DEBUG
  printf("nsWidget::OnDeleteWindow()\n");
#endif   
  nsBaseWidget::OnDestroy();
  
  return DispatchDestroyEvent();
}

PRBool nsWidget::DispatchDestroyEvent(void) {
  PRBool result = PR_FALSE;
  if (nsnull != mEventCallback) {
    nsGUIEvent event(PR_TRUE, NS_DESTROY, this);
    AddRef();
    result = DispatchWindowEvent(event);
    Release();
  }
  return result;
}

PRBool nsWidget::DispatchMouseEvent(nsMouseEvent& aEvent) 
{
  PRBool result = PR_FALSE;
  if (nsnull == mEventCallback && nsnull == mMouseListener) {
    return result;
  }

  
  if (aEvent.message == NS_MOUSE_BUTTON_DOWN &&
      HandlePopup(aEvent.refPoint.x, aEvent.refPoint.y)) {
    return PR_TRUE;
  }

  if (nsnull != mEventCallback) {
    result = DispatchWindowEvent(aEvent);
    return result;
  }
  if (nsnull != mMouseListener) {
    switch (aEvent.message) {
    case NS_MOUSE_MOVE:
      
      break;
    case NS_MOUSE_BUTTON_DOWN:
      result = ConvertStatus(mMouseListener->MousePressed(aEvent));
      break;
    case NS_MOUSE_BUTTON_UP:
      result = ConvertStatus(mMouseListener->MouseReleased(aEvent));
      result = ConvertStatus(mMouseListener->MouseClicked(aEvent));
      break;
    }
  }
  return result;
}

PRBool
nsWidget::OnResize(nsSizeEvent &event)
{
  mBounds.width = event.mWinWidth;
  mBounds.height = event.mWinHeight;

  nsresult result = PR_FALSE;
  if (mEventCallback) {
      result = DispatchWindowEvent(event);
  }
  return result;
}

PRBool nsWidget::DispatchWindowEvent(nsGUIEvent & aEvent)
{
  nsEventStatus status;
  DispatchEvent(&aEvent, status);
  return ConvertStatus(status);
}

PRBool nsWidget::DispatchKeyEvent(nsKeyEvent & aKeyEvent)
{
  if (mEventCallback) 
  {
    return DispatchWindowEvent(aKeyEvent);
  }

  return PR_FALSE;
}














#undef TRACE_EVENTS
#undef TRACE_EVENTS_MOTION
#undef TRACE_EVENTS_PAINT
#undef TRACE_EVENTS_CROSSING
#undef DEBUG

#ifdef DEBUG
void
nsWidget::DebugPrintEvent(nsGUIEvent &   aEvent,
                          Window         aWindow)
{
#ifndef TRACE_EVENTS_MOTION
  if (aEvent.message == NS_MOUSE_MOVE)
  {
    return;
  }
#endif

#ifndef TRACE_EVENTS_PAINT
  if (aEvent.message == NS_PAINT)
  {
    return;
  }
#endif

#ifndef TRACE_EVENTS_CROSSING
  if (aEvent.message == NS_MOUSE_ENTER || aEvent.message == NS_MOUSE_EXIT)
  {
    return;
  }
#endif

  static int sPrintCount=0;

  nsCAutoString eventString;
  eventString.AssignWithConversion(debug_GuiEventToString(&aEvent));
  printf("%4d %-26s(this=%-8p , window=%-8p",
         sPrintCount++,
         (const char *) eventString,
         (void *) this,
         (void *) aWindow);
         
  printf(" , x=%-3d, y=%d)",aEvent.refPoint.x,aEvent.refPoint.y);

  printf("\n");
}
#endif 



NS_IMETHODIMP nsWidget::DispatchEvent(nsGUIEvent * aEvent,
                                      nsEventStatus &aStatus)
{
#ifdef TRACE_EVENTS
  DebugPrintEvent(*aEvent,mBaseWindow);
#endif

  NS_ADDREF(aEvent->widget);

  if (nsnull != mMenuListener) {
    if (NS_MENU_EVENT == aEvent->eventStructType)
      aStatus = mMenuListener->MenuSelected(NS_STATIC_CAST(nsMenuEvent&, *aEvent));
  }

  aStatus = nsEventStatus_eIgnore;
  if (nsnull != mEventCallback) {
    aStatus = (*mEventCallback)(aEvent);
  }

  
  if ((aStatus != nsEventStatus_eIgnore) && (nsnull != mEventListener)) {
    aStatus = mEventListener->ProcessEvent(*aEvent);
  }

  NS_IF_RELEASE(aEvent->widget);

  return NS_OK;
}

void nsWidget::WidgetPut(nsWidget *aWidget)
{
}

void nsWidget::WidgetMove(nsWidget *aWidget)
{
  PR_LOG(XlibScrollingLM, PR_LOG_DEBUG, ("nsWidget::WidgetMove()\n"));
  XMoveWindow(aWidget->mDisplay, aWidget->mBaseWindow,
              aWidget->mRequestedSize.x, aWidget->mRequestedSize.y);
}

void nsWidget::WidgetResize(nsWidget *aWidget)
{
  PR_LOG(XlibScrollingLM, PR_LOG_DEBUG, ("nsWidget::WidgetResize()\n"));
  XResizeWindow(aWidget->mDisplay, aWidget->mBaseWindow,
                aWidget->mRequestedSize.width,
                aWidget->mRequestedSize.height);
}

void nsWidget::WidgetMoveResize(nsWidget *aWidget)
{
  PR_LOG(XlibScrollingLM, PR_LOG_DEBUG, ("nsWidget::WidgetMoveResize()\n"));
  XMoveResizeWindow(aWidget->mDisplay,
                    aWidget->mBaseWindow,
                    aWidget->mRequestedSize.x,
                    aWidget->mRequestedSize.y,
                    aWidget->mRequestedSize.width,
                    aWidget->mRequestedSize.height);
}

void nsWidget::WidgetShow(nsWidget *aWidget)
{
  aWidget->Map();
}

PRBool nsWidget::WidgetVisible(nsRect &aBounds)
{
  nsRect scrollArea;
  scrollArea.x = 0;
  scrollArea.y = 0;
  scrollArea.width = mRequestedSize.width + 1;
  scrollArea.height = mRequestedSize.height + 1;
  if (scrollArea.Intersects(aBounds)) {
    PR_LOG(XlibScrollingLM, PR_LOG_DEBUG, ("nsWidget::WidgetVisible(): widget is visible\n"));
    return PR_TRUE;
  }
  PR_LOG(XlibScrollingLM, PR_LOG_DEBUG, ("nsWidget::WidgetVisible(): widget is not visible\n"));
  return PR_FALSE;
}

void nsWidget::Map(void)
{
  if (!mMapped) {
     
    if (mDisplay)
      XMapWindow(mDisplay, mBaseWindow);
    mMapped = PR_TRUE;
  }
}

void nsWidget::Unmap(void)
{
  if (mMapped) {
     
    if (mDisplay)
      XUnmapWindow(mDisplay, mBaseWindow);
    mMapped = PR_FALSE;
  }
}

void nsWidget::SetVisibility(int aState)
{
  mVisibility = aState;
}


Cursor nsWidget::XlibCreateCursor(nsCursor aCursorType)
{
  Pixmap cursor = None;
  Pixmap mask = None;
  XColor fg, bg;
  Cursor xcursor = None;
  PRUint8 newType = 0xff;

  fg.pixel = 0;
  fg.red = 0;
  fg.green = 0;
  fg.blue = 0;
  fg.flags = 0xf;

  bg.pixel = 0xffffffff;
  bg.red = 0xffff;
  bg.green = 0xffff;
  bg.blue = 0xffff;
  bg.flags = 0xf;

  
  if ((xcursor = gsXlibCursorCache[aCursorType])) {
#ifdef DEBUG_CURSORCACHE
    printf("cached cursor found: %lx\n", xcursor);
#endif
    return xcursor;
  }

  
  switch (aCursorType) {
    case eCursor_select:
      xcursor = XCreateFontCursor(mDisplay, XC_xterm);
      break;
    case eCursor_wait:
      xcursor = XCreateFontCursor(mDisplay, XC_watch);
      break;
    case eCursor_hyperlink:
      xcursor = XCreateFontCursor(mDisplay, XC_hand2);
      break;
    case eCursor_standard:
      xcursor = XCreateFontCursor(mDisplay, XC_left_ptr);
      break;
    case eCursor_n_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_top_side);
      break;
    case eCursor_s_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_bottom_side);
      break;
    case eCursor_w_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_left_side);
      break;
    case eCursor_e_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_right_side);
      break;
    case eCursor_nw_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_top_left_corner);
      break;
    case eCursor_se_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_bottom_right_corner);
      break;
    case eCursor_ne_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_top_right_corner);
      break;
    case eCursor_sw_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_bottom_left_corner);
      break;
    case eCursor_crosshair:
      xcursor = XCreateFontCursor(mDisplay, XC_crosshair);
      break;
    case eCursor_move:
      xcursor = XCreateFontCursor(mDisplay, XC_fleur);
      break;
    case eCursor_help:
      newType = XLIB_QUESTION_ARROW;
      break;
    case eCursor_copy:
      newType = XLIB_COPY;
      break;
    case eCursor_alias:
      newType = XLIB_ALIAS;
      break;
    case eCursor_context_menu:
      newType = XLIB_CONTEXT_MENU;
      break;
    case eCursor_cell:
      xcursor = XCreateFontCursor(mDisplay, XC_plus);
      break;
    case eCursor_grab:
      newType = XLIB_HAND_GRAB;
      break;
    case eCursor_grabbing:
      newType = XLIB_HAND_GRABBING;
      break;
    case eCursor_spinning:
      newType = XLIB_SPINNING;
      break;
    case eCursor_zoom_in:
      newType = XLIB_ZOOM_IN;
      break;
    case eCursor_zoom_out:
      newType = XLIB_ZOOM_OUT;
      break;
    case eCursor_not_allowed:
    case eCursor_no_drop:
      newType = XLIB_NOT_ALLOWED;
      break;
    case eCursor_col_resize:
      newType = XLIB_COL_RESIZE;
      break;
    case eCursor_row_resize:
      newType = XLIB_ROW_RESIZE;
      break;
    case eCursor_vertical_text:
      newType = XLIB_VERTICAL_TEXT;
      break;
    case eCursor_all_scroll:
      xcursor = XCreateFontCursor(mDisplay, XC_fleur);
      break;
    case eCursor_nesw_resize:
      newType = XLIB_NESW_RESIZE;
      break;
    case eCursor_nwse_resize:
      newType = XLIB_NWSE_RESIZE;
      break;
    case eCursor_ns_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_sb_v_double_arrow);
      break;
    case eCursor_ew_resize:
      xcursor = XCreateFontCursor(mDisplay, XC_sb_h_double_arrow);
      break;
    default:
      break;
  }

  
  if (!xcursor) {
    NS_ASSERTION(newType != 0xff, "Unknown cursor type and no standard cursor");
    
    

    cursor = XCreatePixmapFromBitmapData(mDisplay, mBaseWindow,
                        (char *)XlibCursors[newType].bits,
                        32, 32, 0xffffffff, 0x0, 1);

    mask = XCreatePixmapFromBitmapData(mDisplay, mBaseWindow,
                        (char *)XlibCursors[newType].mask_bits,
                        32, 32, 0xffffffff, 0x0, 1);

    xcursor = XCreatePixmapCursor(mDisplay, cursor, mask, &fg, &bg,
                                  XlibCursors[newType].hot_x,
                                  XlibCursors[newType].hot_y);

    XFreePixmap(mDisplay, mask);
    XFreePixmap(mDisplay, cursor);
  }

#ifdef DEBUG_CURSORCACHE
  printf("inserting cursor into the cache: %lx\n", xcursor);
#endif
  gsXlibCursorCache[aCursorType] = xcursor;
  
  return xcursor;
}

void
nsWidget::SetUpWMHints(void) {
  
  if (WMProtocolsInitialized == PR_FALSE) {
    WMDeleteWindow = XInternAtom(mDisplay, "WM_DELETE_WINDOW", True);
    WMTakeFocus = XInternAtom(mDisplay, "WM_TAKE_FOCUS", True);
    WMSaveYourself = XInternAtom(mDisplay, "WM_SAVE_YOURSELF", True);
    WMProtocolsInitialized = PR_TRUE;
  }
  Atom WMProtocols[2];
  WMProtocols[0] = WMDeleteWindow;
  WMProtocols[1] = WMTakeFocus;
  
  PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Setting up wm hints for window 0x%lx\n",
                                       mBaseWindow));
  XSetWMProtocols(mDisplay, mBaseWindow, WMProtocols, 2);
}

NS_METHOD nsWidget::SetBounds(const nsRect &aRect)
{
  mRequestedSize = aRect;
  return nsBaseWidget::SetBounds(aRect);
}

NS_METHOD nsWidget::GetRequestedBounds(nsRect &aRect)
{
  aRect = mRequestedSize;
  return NS_OK;
}

NS_IMETHODIMP
nsWidget::SetTitle(const nsAString& title)
{
  return NS_OK;
}

NS_IMETHODIMP nsWidget::CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent)
{
  return NS_OK;
}

