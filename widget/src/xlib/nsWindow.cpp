










































#include "nsWindow.h"
#include "xlibrgb.h"

#include "nsIRenderingContext.h"


#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsIServiceManager.h"

#define NS_WINDOW_TITLE_MAX_LENGTH 4095


PRBool   nsWindow::sIsGrabbing = PR_FALSE;
nsWindow *nsWindow::sGrabWindow = nsnull;








nsListItem::nsListItem(void *aData, nsListItem *aPrev)
{
  next = nsnull;
  prev = aPrev;
  data = aData;
}

nsList::nsList()
{
  head = nsnull;
  tail = nsnull;
}

nsList::~nsList()
{
  reset();
}

void nsList::add(void *aData)
{
  if (head == nsnull) {           
    head = new nsListItem(aData, nsnull);
    tail = head;
  } else {                        
    tail->setNext(new nsListItem(aData, tail));
    tail = tail->getNext();       
    tail->setNext(nsnull);
  }
}

void nsList::remove(void *aData)
{
  if (head == nsnull) {           
    return;
  } else {                        
    nsListItem *aItem = head;
    while ((aItem != nsnull) && (aItem->getData() != aData)) {
      aItem = aItem->getNext();
    }
    if (aItem == nsnull) {        
      return;
    } else
    if (aItem == head) {          
      head = aItem->getNext();
      delete aItem;
      if (head == nsnull)         
        tail = nsnull;
      else
        head->setPrev(nsnull);
    } else
    if (aItem == tail) {          
      tail = aItem->getPrev();
      delete aItem;
      if (tail == nsnull)         
        head = nsnull;
      else
        tail->setNext(nsnull);
    } else {                      
      nsListItem *prev = aItem->getPrev();
      nsListItem *next = aItem->getNext();
      delete aItem;
      prev->setNext(next);
      next->setPrev(prev);
    }
  }
}

void nsList::reset()
{
  while (head != nsnull) {
    void *aData = head->getData();
    remove(aData);
  }
}

static nsList *update_queue = nsnull;

void 
nsWindow::UpdateIdle (void *data)
{
  if (update_queue != nsnull) {
    nsList     *old_queue;
    nsListItem *tmp_list;

    old_queue    = update_queue;
    update_queue = nsnull;
    
    for( tmp_list = old_queue->getHead() ; tmp_list != nsnull ; tmp_list = tmp_list->getNext() )
    {
      nsWindow *window = NS_STATIC_CAST(nsWindow*,(tmp_list->getData()));       

      window->mIsUpdating = PR_FALSE;
    }
    
    for( tmp_list = old_queue->getHead() ; tmp_list != nsnull ; tmp_list = tmp_list->getNext() )
    {
      nsWindow *window = NS_STATIC_CAST(nsWindow*,(tmp_list->getData()));
       
      window->Update();
    }    

    delete old_queue;
  }
}




NS_IMETHODIMP nsWindow::GetAttention(PRInt32 aCycleCount)
{
  XRaiseWindow(mDisplay, mBaseWindow);
  return NS_OK;
}

void
nsWindow::QueueDraw ()
{
  if (!mIsUpdating)
  {
    if (update_queue == nsnull)
      update_queue = new nsList();
    update_queue->add((void *)this);
    mIsUpdating = PR_TRUE;
  }
}

void
nsWindow::UnqueueDraw ()
{
  if (mIsUpdating)
  {
    if (update_queue != nsnull)
      update_queue->remove((void *)this);
    mIsUpdating = PR_FALSE;
  }
}


NS_IMPL_ISUPPORTS_INHERITED0(nsWindow, nsWidget)


nsWindow::nsWindow() : nsWidget()
{
  mName.AssignLiteral("nsWindow");
  mBackground = NS_RGB(255, 255, 255);
  mBackgroundPixel = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, mBackground);
  mBorderRGB = NS_RGB(255,255,255);
  mBorderPixel = xxlib_rgb_xpixel_from_rgb(mXlibRgbHandle, mBorderRGB);

  
  mIsUpdating = PR_FALSE;
  mBlockFocusEvents = PR_FALSE;
  mLastGrabFailed = PR_TRUE;
  mIsTooSmall = PR_FALSE;

  
  mWindowType = eWindowType_child;
  mBorderStyle = eBorderStyle_default;
  mIsToplevel = PR_FALSE;
  
  mScrollGC = nsnull;
}


nsWindow::~nsWindow()
{
  if (mScrollGC)
    XFreeGC(mDisplay, mScrollGC);
  
  
  if (sGrabWindow == this)
  {
    sIsGrabbing = PR_FALSE;
    sGrabWindow = nsnull;
  }
 
  
  
  
  if (mIsUpdating)
    UnqueueDraw();
}

PRBool nsWindow::OnExpose(nsPaintEvent &event)
{
  nsresult result = PR_TRUE;

  
  if (mEventCallback) 
  {
    event.renderingContext = nsnull;

    

    
    

    
  }

  return result;
}

NS_IMETHODIMP nsWindow::Show(PRBool bState)
{
  
  if (mIsTooSmall)
    return NS_OK;

  if (bState) {
    if (mIsToplevel) {
      PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Someone just used the show method on the toplevel window.\n"));
    }

    if (mParentWidget) {
      ((nsWidget *)mParentWidget)->WidgetShow(this);
      
      XRaiseWindow(mDisplay, mBaseWindow);
    } else {
      if (mBaseWindow) {
        PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Mapping window 0x%lx...\n", mBaseWindow));
        Map();
      }
    }

    mIsShown = PR_TRUE;
    if (sGrabWindow == this && mLastGrabFailed) {
      

      NativeGrab(PR_TRUE);
    }
  } else {
    if (mBaseWindow) {
      PR_LOG(XlibWidgetsLM, PR_LOG_DEBUG, ("Unmapping window 0x%lx...\n", mBaseWindow));
      Unmap();
    }
    mIsShown = PR_FALSE;
  }
  return NS_OK;
}


void nsWindow::NativeGrab(PRBool aGrab)
{
  mLastGrabFailed = PR_FALSE;

  if (aGrab)
  {
    int retval;

    Cursor newCursor = XCreateFontCursor(mDisplay, XC_right_ptr);
    retval = XGrabPointer(mDisplay, mBaseWindow, PR_TRUE, (ButtonPressMask |
                          ButtonReleaseMask | EnterWindowMask | LeaveWindowMask 
                          | PointerMotionMask), GrabModeAsync, GrabModeAsync, 
                          (Window)0, newCursor, CurrentTime);
    XFreeCursor(mDisplay, newCursor);

    if (retval != GrabSuccess)
      mLastGrabFailed = PR_TRUE;

    retval = XGrabKeyboard(mDisplay, mBaseWindow, PR_TRUE, GrabModeAsync,
                           GrabModeAsync, CurrentTime);

    if (retval != GrabSuccess)
      mLastGrabFailed = PR_TRUE;

  } else {
    XUngrabPointer(mDisplay, CurrentTime);
    XUngrabKeyboard(mDisplay, CurrentTime);
  }
}





NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener,
                                            PRBool aDoCapture,
                                            PRBool aConsumeRollupEvent)
  {
  if (aDoCapture) {
    NativeGrab(PR_TRUE);

    sIsGrabbing = PR_TRUE;
    sGrabWindow = this;

    gRollupConsumeRollupEvent = PR_TRUE;
    gRollupListener = aListener;
    gRollupWidget = do_GetWeakReference(NS_STATIC_CAST(nsIWidget*, this));
  }else{
    
    if (sGrabWindow == this)
      sGrabWindow = nsnull;

    sIsGrabbing = PR_FALSE;

    NativeGrab(PR_FALSE);

    gRollupListener = nsnull;
    gRollupWidget = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsWindow::InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous)
{
  mUpdateArea->Union(*aRegion);

  if (aIsSynchronous)
    Update();
  else
    QueueDraw();
  
  return NS_OK;
}


NS_IMETHODIMP nsWindow::SetFocus(PRBool aRaise)
{
  nsEventStatus status;
  nsFocusEvent event(PR_TRUE, NS_GOTFOCUS, this);
  

  if (mBaseWindow)
    mFocusWindow = mBaseWindow;
   
  if (mBlockFocusEvents)
    return NS_OK;
   
  mBlockFocusEvents = PR_TRUE;
 
  AddRef();
  DispatchEvent(&event, status);
  Release();
  
  nsGUIEvent actEvent(PR_TRUE, NS_ACTIVATE, this);
  
  AddRef();
  DispatchWindowEvent(actEvent);
  Release();
  
  mBlockFocusEvents = PR_FALSE;
  
  return NS_OK;
}








NS_IMETHODIMP nsWindow::Resize(PRInt32 aWidth,
                               PRInt32 aHeight,
                               PRBool   aRepaint)
{
  
  PRBool NeedToShow = PR_FALSE;
  


  mBounds.width  = aWidth;
  mBounds.height = aHeight;

  

  
  
  if (aWidth <= 1 || aHeight <= 1)
  {
    aWidth = 1;
    aHeight = 1;
    mIsTooSmall = PR_TRUE;
    Show(PR_FALSE);
  }
  else
  {
    if (mIsTooSmall)
    {
      
      NeedToShow = mIsShown;
      mIsTooSmall = PR_FALSE;
    }
  }
  nsWidget::Resize(aWidth, aHeight, aRepaint);

  nsSizeEvent sevent(PR_TRUE, NS_SIZE, this);
  nsRect sevent_windowSize(0, 0, aWidth, aHeight);
  sevent.windowSize = &sevent_windowSize;
  sevent.mWinWidth = aWidth;
  sevent.mWinHeight = aHeight;
  AddRef();
  OnResize(sevent);
  Release();

  if (NeedToShow)
    Show(PR_TRUE);

  if (aRepaint)
    Invalidate(PR_FALSE);

  return NS_OK;
}






NS_IMETHODIMP nsWindow::Resize(PRInt32 aX,
                               PRInt32 aY,
                               PRInt32 aWidth,
                               PRInt32 aHeight,
                               PRBool   aRepaint)
{
  

  nsWidget::Resize(aX, aY, aWidth, aHeight, aRepaint);

  nsSizeEvent sevent(PR_TRUE, NS_SIZE, this);
  nsRect sevent_windowSize(0, 0, aWidth, aHeight);
  sevent.windowSize = &sevent_windowSize;
  sevent.mWinWidth = aWidth;
  sevent.mWinHeight = aHeight;
  AddRef();
  OnResize(sevent);
  Release();
  return NS_OK;
}

 long
nsWindow::GetEventMask()
{
  long event_mask;

  event_mask = 
    ButtonMotionMask |
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

#undef TRACE_INVALIDATE

#ifdef TRACE_INVALIDATE
static PRInt32 sInvalidatePrintCount = 0;
#endif

NS_IMETHODIMP nsWindow::Invalidate(PRBool aIsSynchronous)
{
  mUpdateArea->SetTo(mBounds.x, mBounds.y, mBounds.width, mBounds.height);

  if (aIsSynchronous)
    Update();
  else
    QueueDraw();

  return NS_OK;
}

NS_IMETHODIMP nsWindow::Invalidate(const nsRect & aRect, PRBool aIsSynchronous)
{
  mUpdateArea->Union(aRect.x, aRect.y, aRect.width, aRect.height);
  if (aIsSynchronous)
    Update();
  else
    QueueDraw();
  
  return NS_OK;
}

void 
nsWindow::DoPaint (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight,
                   nsIRegion *aClipRegion)
{
  if (mEventCallback) {
    nsPaintEvent event(PR_TRUE, NS_PAINT, this);
    nsRect rect(aX, aY, aWidth, aHeight);
    event.refPoint.x = aX;
    event.refPoint.y = aY; 
    event.time = PR_Now(); 
    event.rect = &rect;
    
    event.renderingContext = GetRenderingContext();
    if (event.renderingContext) {
      DispatchWindowEvent(event);
      NS_RELEASE(event.renderingContext);
    }
  }
}

NS_IMETHODIMP nsWindow::Update(void)
{
  if (mIsUpdating)
    UnqueueDraw();

  if (!mUpdateArea->IsEmpty()) {
    PRUint32 numRects;
    mUpdateArea->GetNumRects(&numRects);

    

    if (numRects != 1 && numRects < 16) {
      nsRegionRectSet *regionRectSet = nsnull;

      if (NS_FAILED(mUpdateArea->GetRects(&regionRectSet)))
        return NS_ERROR_FAILURE;

      PRUint32 len;
      PRUint32 i;
      
      len = regionRectSet->mRectsLen;

      for (i=0;i<len;++i) {
        nsRegionRect *r = &(regionRectSet->mRects[i]);
        DoPaint (r->x, r->y, r->width, r->height, mUpdateArea);
      }
      
      mUpdateArea->FreeRects(regionRectSet);
      
      mUpdateArea->SetTo(0, 0, 0, 0);
      return NS_OK;
    } else {
      PRInt32 x, y, w, h;
      mUpdateArea->GetBoundingBox(&x, &y, &w, &h);
      DoPaint (x, y, w, h, mUpdateArea);
      mUpdateArea->SetTo(0, 0, 0, 0);
    }
  } 

  
  
  
  return NS_OK;
}

NS_IMETHODIMP nsWindow::Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect)
{
  PR_LOG(XlibScrollingLM, PR_LOG_DEBUG, ("nsWindow::Scroll()\n"));
  
  if (mIsUpdating)
    UnqueueDraw();

  PRInt32 srcX = 0, srcY = 0, destX = 0, destY = 0, width = 0, height = 0;
  nsRect aRect;
 
  
  if (!mScrollGC)
    mScrollGC = XCreateGC(mDisplay, mBaseWindow, 0, nsnull);   
 
  if (aDx < 0 || aDy < 0)
  {
    srcX   = mBounds.x + PR_ABS(aDx);
    srcY   = mBounds.y + PR_ABS(aDy);
    destX  = mBounds.x;
    destY  = mBounds.y;
    width  = mBounds.width  - PR_ABS(aDx);
    height = mBounds.height - PR_ABS(aDy);
  } 
  else if (aDx > 0 || aDy > 0)
  {
    srcX   = mBounds.x;
    srcY   = mBounds.y;
    destX  = mBounds.x + PR_ABS(aDx);
    destY  = mBounds.y + PR_ABS(aDy);
    width  = mBounds.width  - PR_ABS(aDx);
    height = mBounds.height - PR_ABS(aDy);
  }

  XCopyArea(mDisplay, mBaseWindow, mBaseWindow, mScrollGC,
            srcX, srcY, width, height, destX, destY);

  width = mBounds.width;
  height = mBounds.height;

  if (aDx != 0 || aDy != 0) {
    if (aDx < 0) {
      aRect.SetRect(width + aDx, 0,
                    -aDx, height);
    }
    else if (aDx > 0) {
      aRect.SetRect(0,0, aDx, height);
    }

    if (aDy < 0) {
      aRect.SetRect(0, height + aDy,
                    width, -aDy);
    }
    else if (aDy > 0) {
      aRect.SetRect(0,0, width, aDy);
    }

    mUpdateArea->Offset(aDx, aDy);
    Invalidate(aRect, PR_TRUE);
  }

  
  
  for (nsIWidget* kid = mFirstChild; kid; kid = kid->GetNextSibling()) {
    nsWindow* childWindow = NS_STATIC_CAST(nsWindow*, kid);
    nsRect bounds;
    childWindow->GetRequestedBounds(bounds);
    childWindow->Move(bounds.x + aDx, bounds.y + aDy);
    Invalidate(bounds, PR_TRUE);
  }

  
  

  if (mVisibility == VisibilityPartiallyObscured)
  {
    XEvent event;
    PRBool needToUpdate = PR_FALSE;
    mUpdateArea->SetTo(0,0,0,0);
    while(XCheckWindowEvent(mDisplay, mBaseWindow, 0xffffffff, &event))
    {
      if (event.type == GraphicsExpose) {
        nsRect rect;
        rect.SetRect(event.xgraphicsexpose.x,
                     event.xgraphicsexpose.y,
                     event.xgraphicsexpose.width,
                     event.xgraphicsexpose.height);
        mUpdateArea->Union(rect.x, rect.y, rect.width, rect.height);
        needToUpdate = PR_TRUE;
        if (event.xgraphicsexpose.count == 0) {
          continue;
        }
      }
    }
    if (needToUpdate)
      Update();
  }
  return NS_OK;
}

NS_IMETHODIMP nsWindow::ScrollWidgets(PRInt32 aDx, PRInt32 aDy)
{
  return Scroll(aDx, aDy, nsnull);
}

NS_IMETHODIMP nsWindow::ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy)
{
  return Scroll(aDx, aDy, nsnull);
}

NS_IMETHODIMP nsWindow::SetTitle(const nsAString& aTitle)
{
  if(!mBaseWindow)
    return NS_ERROR_FAILURE;

  nsresult rv;
  char *platformText;
  PRInt32 platformLen;

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
      rv = encoder->Convert(title, &len, platformText, &platformLen);
      (platformText)[platformLen] = '\0';  
    }
  } 

  if (platformLen > 0) {
    int status = 0;
    XTextProperty prop;

    
    prop.value = 0;
    status = XmbTextListToTextProperty(mDisplay, &platformText, 1,
        XStdICCTextStyle, &prop);

    if (status == Success) {
      XSetWMProperties(mDisplay, mBaseWindow,
                       &prop, &prop, nsnull, 0, nsnull, nsnull, nsnull);
      if (prop.value)
        XFree(prop.value);

      nsMemory::Free(platformText);
      return NS_OK;
    } else {                    
      if (prop.value)
        XFree(prop.value);
      nsMemory::Free(platformText);
    }
  }

  
  XStoreName(mDisplay, mBaseWindow, NS_LossyConvertUTF16toASCII(aTitle).get());

  return NS_OK;
}

ChildWindow::ChildWindow(): nsWindow()
{
  mName.AssignLiteral("nsChildWindow");
}


