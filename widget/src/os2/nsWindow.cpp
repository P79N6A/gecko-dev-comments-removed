











































#include "nsWindow.h"
#include "nsIAppShell.h"
#include "nsGUIEvent.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIScreenManager.h"
#include "nsRect.h"
#include "nsTransform2D.h"
#include "nsGfxCIID.h"
#include "prtime.h"
#include "nsHashtable.h"
#include "nsDragService.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsWidgetAtoms.h"

#include "nsIRollupListener.h"
#include "nsIMenuRollup.h"
#include "nsIRegion.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"


#include "npapi.h"

#include "nsITimer.h"
#include "nsIServiceManager.h"


#include "nsAppDirectoryServiceDefs.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"

#include "nsOS2Uni.h"

#include "imgIContainer.h"

#include "nsTHashtable.h"
#include "nsHashKeys.h"

#include <stdlib.h>
#include <ctype.h>

#include "nsdefs.h"
#include "wdgtos2rc.h"

#ifdef DEBUG_sobotka
static int WINDOWCOUNT = 0;
#endif

static const char *sScreenManagerContractID = "@mozilla.org/gfx/screenmanager;1";




















nsWindow* nsWindow::gCurrentWindow = nsnull;
BOOL nsWindow::sIsRegistered       = FALSE;




nsIRollupListener * gRollupListener           = nsnull;
nsIWidget         * gRollupWidget             = nsnull;
PRBool              gRollupConsumeRollupEvent = PR_FALSE;






static POINTS gLastButton1Down = {0,0};

#define XFROMMP(m)    (SHORT(LOUSHORT(m)))
#define YFROMMP(m)    (SHORT(HIUSHORT(m)))
 

static PRBool gGlobalsInitialized = PR_FALSE;
static HPOINTER gPtrArray[IDC_COUNT];
static PRBool gIsTrackPoint = PR_FALSE;
static PRBool gIsDBCS = PR_FALSE;

#ifdef DEBUG_FOCUS
int currentWindowIdentifier = 0;
#endif






#define ACTION_PAINT    1
#define ACTION_DRAW     2
#define ACTION_SCROLL   3
#define ACTION_SHOW     4
#define ACTION_PTRPOS   5


#define DND_None                (nsIDragSessionOS2::DND_NONE)               
#define DND_NativeDrag          (nsIDragSessionOS2::DND_NATIVEDRAG)         
#define DND_MozDrag             (nsIDragSessionOS2::DND_MOZDRAG)            
#define DND_InDrop              (nsIDragSessionOS2::DND_INDROP)             
#define DND_DragStatus          (nsIDragSessionOS2::DND_DRAGSTATUS)         
#define DND_DispatchEnterEvent  (nsIDragSessionOS2::DND_DISPATCHENTEREVENT) 
#define DND_DispatchEvent       (nsIDragSessionOS2::DND_DISPATCHEVENT)      
#define DND_GetDragoverResult   (nsIDragSessionOS2::DND_GETDRAGOVERRESULT)  
#define DND_ExitSession         (nsIDragSessionOS2::DND_EXITSESSION)        


static PRUint32  gDragStatus = 0;





#define WM_APPCOMMAND  0x0319

#define APPCOMMAND_BROWSER_BACKWARD       1
#define APPCOMMAND_BROWSER_FORWARD        2
#define APPCOMMAND_BROWSER_REFRESH        3
#define APPCOMMAND_BROWSER_STOP           4

#define FAPPCOMMAND_MASK  0xF000
#define GET_APPCOMMAND_LPARAM(lParam) ((USHORT)(HIUSHORT(lParam) & ~FAPPCOMMAND_MASK))


#define NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION "MozillaPluginWindowPropertyAssociation"






nsWindow::nsWindow() : nsBaseWidget()
{
    mWnd                = 0;
    mFrameWnd           = 0;
    mPrevWndProc        = NULL;
    mParent             = 0;
    mNextID             = 1;
    mFrameIcon          = 0;
    mDeadKey            = 0;
    mHaveDeadKey        = FALSE;
    mIsDestroying       = PR_FALSE;
    mOnDestroyCalled    = PR_FALSE;

    mWindowState        = nsWindowState_ePrecreate;
    mWindowType         = eWindowType_child;
    mBorderStyle        = eBorderStyle_default;
    mOS2Toolkit         = nsnull;
    mIsScrollBar         = FALSE;
    mInSetFocus         = FALSE;
    mChromeHidden       = FALSE;
    mDragHps            = 0;
    mDragStatus         = 0;
    mCssCursorHPtr      = 0;
    mClipWnd            = 0;

    mIsTopWidgetWindow = PR_FALSE;
    mThebesSurface = nsnull;

    if (!gGlobalsInitialized) {
      gGlobalsInitialized = PR_TRUE;
      HMODULE hModResources = NULLHANDLE;
      DosQueryModFromEIP(&hModResources, NULL, 0, NULL, NULL, (ULONG) &gGlobalsInitialized);
      for (int i = 0; i < IDC_COUNT; i++) {
        gPtrArray[i] = ::WinLoadPointer(HWND_DESKTOP, hModResources, IDC_BASE+i);
      }

      
      char buffer[CCHMAXPATH];
      COUNTRYCODE cc = { 0 };
      DosQueryDBCSEnv( CCHMAXPATH, &cc, buffer);
      gIsDBCS = buffer[0] || buffer[1];

      
      
      
      
      
      nsresult rv;
      nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
      if (NS_SUCCEEDED(rv) && prefs)
         prefs->GetBoolPref("os2.trackpoint", &gIsTrackPoint);
    }
}






nsWindow::~nsWindow()
{
  
  
  
  
  
  
  
  
  mIsDestroying = PR_TRUE;
  if (gCurrentWindow == this) {
    gCurrentWindow = nsnull;
  }
  if (mFrameIcon) {
     WinFreeFileIcon(mFrameIcon);
     mFrameIcon = NULLHANDLE;
  }

  if (mCssCursorHPtr) {
    WinDestroyPointer(mCssCursorHPtr);
    mCssCursorHPtr = 0;
  }

  
  
  if( !(mWindowState & nsWindowState_eDead) )
  {
    mWindowState |= nsWindowState_eDoingDelete;
    mWindowState &= ~(nsWindowState_eLive|nsWindowState_ePrecreate|
                      nsWindowState_eInCreate);
    Destroy();
  }

}

 void
nsWindow::ReleaseGlobals()
{
  for (int i = 0; i < IDC_COUNT; i++) {
    WinDestroyPointer(gPtrArray[i]);
  }
}

NS_METHOD nsWindow::CaptureMouse(PRBool aCapture)
{
  if (PR_TRUE == aCapture) { 
    WinSetCapture( HWND_DESKTOP, mWnd);
  } else {
    WinSetCapture( HWND_DESKTOP, NULLHANDLE);
  }
  return NS_OK;
}

nsIntPoint nsWindow::WidgetToScreenOffset()
{
  POINTL point = { 0, 0 };
  NS2PM( point);

  WinMapWindowPoints( mWnd, HWND_DESKTOP, &point, 1);

  return nsIntPoint(point.x,
                    WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN) - point.y - 1);
}






void nsWindow::InitEvent(nsGUIEvent& event, nsIntPoint* aPoint)
{
  
  if (nsnull == aPoint) {
    
    
    POINTL ptl;
    if (CheckDragStatus(ACTION_PTRPOS, 0))
      WinQueryPointerPos( HWND_DESKTOP, &ptl);
    else
      WinQueryMsgPos( 0, &ptl);

    WinMapWindowPoints( HWND_DESKTOP, mWnd, &ptl, 1);
    PM2NS( ptl);
    event.refPoint.x = ptl.x;
    event.refPoint.y = ptl.y;
  }
  else
  
  {
    event.refPoint.x = aPoint->x;
    event.refPoint.y = aPoint->y;
  }

  event.time = WinQueryMsgTime( 0);
  return;
}







NS_IMETHODIMP nsWindow::DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus)
{
#if defined(TRACE_EVENTS) && defined(DEBUG_sobotka)
  DebugPrintEvent(*event, mWnd);
#endif

  aStatus = nsEventStatus_eIgnore;

  
  
  
  if( ((mWindowState == nsWindowState_eInCreate) && event->message == NS_CREATE)
      || (mWindowState & nsWindowState_eLive) )
  {
    if (nsnull != mEventCallback) {
      aStatus = (*mEventCallback)( event);
    }
  }

  return NS_OK;
}


PRBool nsWindow::DispatchWindowEvent(nsGUIEvent* event)
{
  nsEventStatus status;
  DispatchEvent(event, status);
  return ConvertStatus(status);
}

PRBool nsWindow::DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus) {
  DispatchEvent(event, aStatus);
  return ConvertStatus(aStatus);
}







PRBool nsWindow::DispatchStandardEvent(PRUint32 aMsg)
{
  nsGUIEvent event(PR_TRUE, aMsg, this);
  InitEvent(event);

  return DispatchWindowEvent(&event);
}






PRBool nsWindow::DispatchCommandEvent(PRUint32 aEventCommand)
{
  nsCOMPtr<nsIAtom> command;
  switch (aEventCommand) {
    case APPCOMMAND_BROWSER_BACKWARD:
      command = nsWidgetAtoms::Back;
      break;
    case APPCOMMAND_BROWSER_FORWARD:
      command = nsWidgetAtoms::Forward;
      break;
    case APPCOMMAND_BROWSER_REFRESH:
      command = nsWidgetAtoms::Reload;
      break;
    case APPCOMMAND_BROWSER_STOP:
      command = nsWidgetAtoms::Stop;
      break;
    default:
      return PR_FALSE;
  }
  nsCommandEvent event(PR_TRUE, nsWidgetAtoms::onAppCommand, command, this);

  InitEvent(event);
  return DispatchWindowEvent(&event);
}







PRBool nsWindow::DispatchDragDropEvent(PRUint32 aMsg)
{
  nsDragEvent event(PR_TRUE, aMsg, this);
  InitEvent(event);

  event.isShift   = WinIsKeyDown(VK_SHIFT);
  event.isControl = WinIsKeyDown(VK_CTRL);
  event.isAlt     = WinIsKeyDown(VK_ALT) || WinIsKeyDown(VK_ALTGRAF);
  event.isMeta    = PR_FALSE;

  return DispatchWindowEvent(&event);
}


NS_IMETHODIMP nsWindow::CaptureRollupEvents(nsIRollupListener * aListener, 
                                            PRBool aDoCapture, 
                                            PRBool aConsumeRollupEvent)
{
  if (aDoCapture) {
    


    NS_ASSERTION(!gRollupWidget, "rollup widget reassigned before release");
    gRollupConsumeRollupEvent = aConsumeRollupEvent;
    NS_IF_RELEASE(gRollupListener);
    NS_IF_RELEASE(gRollupWidget);
    gRollupListener = aListener;
    NS_ADDREF(aListener);
    gRollupWidget = this;
    NS_ADDREF(this);
  } else {
    NS_IF_RELEASE(gRollupListener);
    
    NS_IF_RELEASE(gRollupWidget);
  }

  return NS_OK;
}

PRBool 
nsWindow::EventIsInsideWindow(nsWindow* aWindow) 
{
  RECTL r;
  POINTL mp;
  if (WinQueryMsgPos( 0, &mp)) {
    WinMapWindowPoints( HWND_DESKTOP, aWindow->mWnd, &mp, 1);
    WinQueryWindowRect( aWindow->mWnd, &r);
    
    if (mp.x < r.xLeft || mp.x > r.xRight ||
       mp.y > r.yTop || mp.y < r.yBottom) {
      return PR_FALSE;
    }
  } 

  return PR_TRUE;
}


static PCSZ GetNSWindowPropName() {
  static ATOM atom = 0;

  
  
  
  if (!atom) {
    atom = WinAddAtom(WinQuerySystemAtomTable(), "nsWindowPtr");
  }
  return (PCSZ)atom;
}

nsWindow * nsWindow::GetNSWindowPtr(HWND aWnd) {
  return (nsWindow *) ::WinQueryProperty(aWnd, GetNSWindowPropName());
}

BOOL nsWindow::SetNSWindowPtr(HWND aWnd, nsWindow * ptr) {
  if (ptr == NULL) {
    ::WinRemoveProperty(aWnd, GetNSWindowPropName());
    return TRUE;
  } else {
    return ::WinSetProperty(aWnd, GetNSWindowPropName(), (PVOID)ptr, 0);
  }
}






BOOL
nsWindow :: DealWithPopups ( ULONG inMsg, MRESULT* outResult )
{
  if ( gRollupListener && gRollupWidget) {
    if(inMsg == WM_BUTTON1DOWN ||
        inMsg == WM_BUTTON2DOWN || inMsg == WM_BUTTON3DOWN) {
      
      PRBool rollup = !nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget);
      
      
      
      PRUint32 popupsToRollup = PR_UINT32_MAX;
      if (rollup) {
        nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(gRollupListener) );
        if ( menuRollup ) {
          nsAutoTArray<nsIWidget*, 5> widgetChain;
          PRUint32 sameTypeCount = menuRollup->GetSubmenuWidgetChain(&widgetChain);
          for ( PRUint32 i = 0; i < widgetChain.Length(); ++i ) {
            nsIWidget* widget = widgetChain[i];
            if ( nsWindow::EventIsInsideWindow((nsWindow*)widget) ) {
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
      }

      
      if ( rollup ) {
        
        gRollupListener->Rollup(popupsToRollup, inMsg == WM_BUTTON1DOWN ? &mLastRollup : nsnull);

        
        
        
        
        if (gRollupConsumeRollupEvent) {
          *outResult = (MRESULT)TRUE;
          return TRUE;
        } 
      }
    } 
  } 

  return FALSE;

} 




BOOL bothFromSameWindow( HWND hwnd1, HWND hwnd2 )
{
  HWND hwnd1Chain = WinQueryWindow( hwnd1, QW_OWNER );
  if (!hwnd1Chain)
    hwnd1Chain = WinQueryWindow( hwnd1, QW_PARENT );
  HWND hwnd1GChain = WinQueryWindow( hwnd1Chain, QW_OWNER );
  if (!hwnd1GChain)
    hwnd1GChain = WinQueryWindow( hwnd1Chain, QW_PARENT );
  HWND hwnd2Chain = WinQueryWindow( hwnd2, QW_OWNER );
  if (!hwnd2Chain)
    hwnd2Chain = WinQueryWindow( hwnd2, QW_PARENT );
  HWND hwnd2GChain = WinQueryWindow( hwnd2Chain, QW_OWNER );
  if (!hwnd2GChain)
    hwnd2GChain = WinQueryWindow( hwnd2Chain, QW_PARENT );
  while( hwnd1GChain) {
    hwnd1 = hwnd1Chain;
    hwnd1Chain = hwnd1GChain;
    hwnd1GChain = WinQueryWindow( hwnd1Chain, QW_OWNER );
    if (!hwnd1GChain)
      hwnd1GChain = WinQueryWindow( hwnd1Chain, QW_PARENT );
  }
  while( hwnd2GChain) {
    hwnd2 = hwnd2Chain;
    hwnd2Chain = hwnd2GChain;
    hwnd2GChain = WinQueryWindow( hwnd2Chain, QW_OWNER );
    if (!hwnd2GChain)
      hwnd2GChain = WinQueryWindow( hwnd2Chain, QW_PARENT );
  }
  return (hwnd1 == hwnd2);
}






MRESULT EXPENTRY fnwpNSWindow( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   nsAutoRollup autoRollup;

   MRESULT popupHandlingResult;
   if( nsWindow::DealWithPopups(msg, &popupHandlingResult) )
      return popupHandlingResult;

   
   nsWindow *wnd = nsWindow::GetNSWindowPtr(hwnd);

   
   if( nsnull != gRollupListener && nsnull != gRollupWidget) {
      if( msg == WM_ACTIVATE || msg == WM_BUTTON1DOWN ||
          msg == WM_BUTTON2DOWN || msg == WM_BUTTON3DOWN) {
      
      PRBool rollup = !nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget);
      
      
      
      if (rollup) {
        nsCOMPtr<nsIMenuRollup> menuRollup ( do_QueryInterface(gRollupListener) );
        if ( menuRollup ) {
          nsAutoTArray<nsIWidget*, 5> widgetChain;
          menuRollup->GetSubmenuWidgetChain ( &widgetChain );
          for ( PRUint32 i = 0; i < widgetChain.Length(); ++i ) {
            nsIWidget* widget = widgetChain[i];
            if ( nsWindow::EventIsInsideWindow((nsWindow*)widget) ) {
              rollup = PR_FALSE;
              break;
            }
          } 
        } 
      }
      }
      else if( msg == WM_SETFOCUS) {
         if( !mp2 && 
             !bothFromSameWindow( ((nsWindow*)gRollupWidget)->GetMainWindow(), 
                                  (HWND)mp1) ) {
            gRollupListener->Rollup(PR_UINT32_MAX, nsnull);
         }
      }
   }

   
   
   
   switch( msg)
   {
      case WM_CONTROL:
      case WM_HSCROLL:
      case WM_VSCROLL: 
      {
         
         HWND hwndChild = WinWindowFromID( hwnd, SHORT1FROMMP( mp1));
         if( hwndChild)
         {
            nsWindow *w = nsWindow::GetNSWindowPtr(hwndChild);
            if( w)
               wnd = w;
         }
         break;
      }
   }

    
    
    
    nsCOMPtr<nsISupports> kungFuDeathGrip;
    if (!wnd->mIsDestroying) 
      kungFuDeathGrip = do_QueryInterface((nsBaseWidget*)wnd);

   MRESULT mresult = 0;

   if (wnd)
   {
      if( PR_FALSE == wnd->ProcessMessage( msg, mp1, mp2, mresult) &&
          WinIsWindow( (HAB)0, hwnd) && wnd->GetPrevWP())
      {
         mresult = (wnd->GetPrevWP())( hwnd, msg, mp1, mp2);

      }
   }
   else
       mresult = WinDefWindowProc( hwnd, msg, mp1, mp2);

   return mresult;
}






void nsWindow::DoCreate( HWND hwndP, nsWindow *aParent,
                      const nsIntRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
   mWindowState = nsWindowState_eInCreate;

  if( aInitData && (aInitData->mWindowType == eWindowType_dialog ||
                    aInitData->mWindowType == eWindowType_toplevel ||
                    aInitData->mWindowType == eWindowType_invisible))
    mIsTopWidgetWindow = PR_TRUE;
  else
    mIsTopWidgetWindow = PR_FALSE;

   if( aInitData != nsnull) {
     mWindowType = aInitData->mWindowType;
     mBorderStyle = aInitData->mBorderStyle;
   }

   
   if( !mToolkit)
   {
      if( aToolkit)
      {
         mToolkit = aToolkit;
         NS_ADDREF(mToolkit);
      }
      else if( aParent)
      {
         mToolkit = aParent->GetToolkit();
         NS_IF_ADDREF(mToolkit);
      }
      else
      {
         
         
         mToolkit = new nsToolkit;
         NS_ADDREF(mToolkit);
         mToolkit->Init( PR_GetCurrentThread());
      }
      mOS2Toolkit = (nsToolkit*) mToolkit;
   }

   
   RealDoCreate( hwndP, aParent, aRect, aHandleEventFunction,
                 aContext, aAppShell, aInitData);

   mWindowState = nsWindowState_eLive;
}

void nsWindow::RealDoCreate( HWND              hwndP,
                             nsWindow         *aParent,
                             const nsIntRect  &aRect,
                             EVENT_CALLBACK    aHandleEventFunction,
                             nsIDeviceContext *aContext,
                             nsIAppShell      *aAppShell,
                             nsWidgetInitData *aInitData,
                             HWND              hwndOwner)
{
   
   mParent = aParent;

   ULONG style = WindowStyle();
   if( aInitData)
   {
      if( aInitData->clipChildren)
         style |= WS_CLIPCHILDREN;
#if 0
      
      
      
      
      
      
      else
        style &= ~WS_CLIPCHILDREN;
#endif

      if( aInitData->clipSiblings)
         style |= WS_CLIPSIBLINGS;
      else
         style &= ~WS_CLIPSIBLINGS;
   }

   mIsScrollBar = (!(strcmp( WindowClass(), WC_SCROLLBAR_STRING )));

   if( hwndP != HWND_DESKTOP)
   {
      
      if( aInitData && aInitData->mWindowType == eWindowType_popup)
      {
         if( !hwndOwner)
         {
            hwndOwner = hwndP;
         }
         hwndP = HWND_DESKTOP;
      }
      
      else if(!hwndOwner )
      {
         if( mIsScrollBar )
         {
            hwndOwner = hwndP;
         }
      }
   }

#ifdef DEBUG_FOCUS
   mWindowIdentifier = currentWindowIdentifier;
   currentWindowIdentifier++;
   if (aInitData && (aInitData->mWindowType == eWindowType_toplevel))
     DEBUGFOCUS(Create Frame Window);
   else
     DEBUGFOCUS(Create Window);
#endif

   
   
   mWnd = WinCreateWindow( hwndP,
                           WindowClass(),
                           0,          
                           style,
                           0, 0, 0, 0, 
                           hwndOwner,
                           HWND_TOP,
                           mParent ? mParent->GetNextID() : 0,
                           0, 0);      

   NS_ASSERTION( mWnd, "Couldn't create window");

   if (gIsTrackPoint && mWindowType == eWindowType_child && !mIsScrollBar) {
     WinCreateWindow(mWnd, WC_SCROLLBAR, 0, SBS_VERT,
                     0, 0, 0, 0, mWnd, HWND_TOP,
                     FID_VERTSCROLL, NULL, NULL);
   }

#if DEBUG_sobotka
   printf("\n+++++++++++In nsWindow::RealDoCreate created 0x%lx, %d x %d\n",
          mWnd, aRect.width, aRect.height);
   printf("+++++++++++Location =  %d x %d\n", aRect.x, aRect.y);
   printf("+++++++++++Parent = 0x%lx\n", GetParentHWND());
   printf("+++++++++++WINDOWCOUNT+ = %d\n", ++WINDOWCOUNT);
#endif

   
   if( aContext)
   {
      mContext = aContext;
      NS_ADDREF(mContext);
   }
   else
   {
      nsresult rc;
      static NS_DEFINE_IID(kDeviceContextCID, NS_DEVICE_CONTEXT_CID);

      rc = CallCreateInstance(kDeviceContextCID, &mContext);
      if( NS_SUCCEEDED(rc))
         mContext->Init(this);
#ifdef DEBUG
      else
         printf( "Couldn't find DC instance for nsWindow\n");
#endif
   }

   Resize( aRect.x, aRect.y, aRect.width, aRect.height, PR_FALSE);

   
   
   
   
   mBounds = aRect;
   mBounds.height = aRect.height;
   mEventCallback = aHandleEventFunction;

   if( mParent)
      mParent->AddChild( this);

   mContentType = aInitData? aInitData->mContentType : eContentTypeInherit;
   if (mContentType == eContentTypeInherit && aParent) {
      mContentType = aParent->mContentType;
   }

   

   DispatchStandardEvent( NS_CREATE );
   SubclassWindow(TRUE);
   PostCreateWidget();
}






NS_METHOD nsWindow::Create(nsIWidget *aParent,
                           nsNativeWidget aNativeParent,
                           const nsIntRect &aRect,
                           EVENT_CALLBACK aHandleEventFunction,
                           nsIDeviceContext *aContext,
                           nsIAppShell *aAppShell,
                           nsIToolkit *aToolkit,
                           nsWidgetInitData *aInitData)
{
  HWND hwndP;
  nsWindow *pParent;

  if (aParent) {
    hwndP = (HWND)aParent->GetNativeData(NS_NATIVE_WINDOW);
    pParent = (nsWindow*)aParent;
  } else {
    if (aNativeParent && (HWND)aNativeParent != HWND_DESKTOP) {
      hwndP = (HWND)aNativeParent;
      pParent = GetNSWindowPtr(hwndP);
    } else {
      hwndP = HWND_DESKTOP;
      pParent = 0;
    }
  }

  DoCreate(hwndP, pParent, aRect, aHandleEventFunction,
           aContext, aAppShell, aToolkit, aInitData);

  return NS_OK;
}






gfxASurface* nsWindow::GetThebesSurface()
{
  if (mWnd && !mThebesSurface) {
    mThebesSurface = new gfxOS2Surface(mWnd);
  }
  return mThebesSurface;
}






NS_METHOD nsWindow::Destroy()
{
  
  
  if ((mWindowState & nsWindowState_eLive) && mParent) {
    nsBaseWidget::Destroy();
  }

  
  
  if (this == gRollupWidget) {
    if (gRollupListener) {
      gRollupListener->Rollup(PR_UINT32_MAX, nsnull);
    }
    CaptureRollupEvents(nsnull, PR_FALSE, PR_TRUE);
  }

  if (mWnd) {
    HWND hwndBeingDestroyed = mFrameWnd ? mFrameWnd : mWnd;
    DEBUGFOCUS(Destroy);
    if (hwndBeingDestroyed == WinQueryFocus(HWND_DESKTOP)) {
      WinSetFocus(HWND_DESKTOP, WinQueryWindow(hwndBeingDestroyed, QW_PARENT));
    }
    WinDestroyWindow(hwndBeingDestroyed);
  }
  return NS_OK;
}






nsIWidget* nsWindow::GetParent(void)
{
    if (mIsTopWidgetWindow) {
       
       
       
      return nsnull;
    }
    


    if (mIsDestroying || mOnDestroyCalled)
      return nsnull;

   nsWindow *widget = nsnull;
   if ((nsnull != mParent) && (!mParent->mIsDestroying))
   {
      widget = mParent;
   }

   return widget;
}


HWND nsWindow::GetParentHWND() const
{
   HWND hwnd = 0;
   if( nsnull != mParent)
      hwnd = mParent->mWnd;
   else
      hwnd = WinQueryWindow( GetMainWindow(), QW_PARENT);
   return hwnd;
}


void nsWindow::NS2PM_PARENT( POINTL &ptl)
{
   if( mParent)
      mParent->NS2PM( ptl);
   else
   {
      HWND hwndp = WinQueryWindow( GetMainWindow(), QW_PARENT);
      SWP  swp = { 0 };
      WinQueryWindowPos( hwndp, &swp);
      ptl.y = swp.cy - ptl.y - 1;
   }
}


void nsWindow::NS2PM( POINTL &ptl)
{
   ptl.y = GetClientHeight() - ptl.y - 1;
#if 0
   printf("+++++++++In NS2PM client height = %d\n", GetClientHeight());
#endif
}


void nsWindow::NS2PM( RECTL &rcl)
{
   LONG height = rcl.yTop - rcl.yBottom;
   rcl.yTop = GetClientHeight() - rcl.yBottom;
   rcl.yBottom = rcl.yTop - height;
}






NS_METHOD nsWindow::Show(PRBool bState)
{
  
  if (mWnd) {
    if (bState) {
      
      
      if (CheckDragStatus(ACTION_SHOW, 0)) {
        PRBool bVisible;
        IsVisible(bVisible);
        if (!bVisible)
          PlaceBehind(eZPlacementTop, NULL, PR_FALSE);
        WinShowWindow(mWnd, PR_TRUE);
      }
    } else {
      WinShowWindow(mWnd, PR_FALSE);
    }
  }

  return NS_OK;
}






NS_METHOD nsWindow::IsVisible(PRBool & bState)
{
   
   bState = WinIsWindowVisible( mWnd) ? PR_TRUE : PR_FALSE;
   return NS_OK;
}






NS_METHOD nsWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                nsIWidget *aWidget, PRBool aActivate)
{
  HWND behind = HWND_TOP;
  if (aPlacement == eZPlacementBottom)
    behind = HWND_BOTTOM;
  else if (aPlacement == eZPlacementBelow && aWidget)
    behind = (HWND)aWidget->GetNativeData(NS_NATIVE_WINDOW);
  UINT flags = SWP_ZORDER;
  if (aActivate)
    flags |= SWP_ACTIVATE;

  WinSetWindowPos(mWnd, behind, 0, 0, 0, 0, flags);
  return NS_OK;
}






NS_METHOD nsWindow::SetZIndex(PRInt32 aZIndex)
{
  
  
  
  return NS_OK;
}











NS_IMETHODIMP nsWindow::SetSizeMode(PRInt32 aMode)
{
  PRInt32 previousMode;
  GetSizeMode(&previousMode);

  
  nsresult rv = nsBaseWidget::SetSizeMode(aMode);

  
  
  
  if (previousMode == nsSizeMode_Minimized && previousMode != aMode) {
    DEBUGFOCUS(deferred NS_ACTIVATE);
    ActivateTopLevelWidget();
  }

  
  if (!NS_SUCCEEDED(rv) || !mChromeHidden || aMode == nsSizeMode_Maximized)
    return rv;

  HWND  hFrame = GetMainWindow();
  ULONG ulStyle = WinQueryWindowULong( hFrame, QWL_STYLE);

  
  if (aMode == nsSizeMode_Minimized) {
    if (!(ulStyle & WS_MINIMIZED))
      WinSetWindowPos(hFrame, 0, 0, 0, 0, 0, SWP_MINIMIZE | SWP_DEACTIVATE);
  }
  else
    if (ulStyle & (WS_MAXIMIZED | WS_MINIMIZED))
      WinSetWindowPos(hFrame, 0, 0, 0, 0, 0, SWP_RESTORE);

  return NS_OK;
}






NS_METHOD nsWindow::ConstrainPosition(PRBool aAllowSlop,
                                      PRInt32 *aX, PRInt32 *aY)
{
  if (!mIsTopWidgetWindow) 
    return NS_OK;

  PRBool doConstrain = PR_FALSE; 

  

  RECTL screenRect;

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    PRInt32 left, top, width, height;

    
    width = mBounds.width > 0 ? mBounds.width : 1;
    height = mBounds.height > 0 ? mBounds.height : 1;
    screenmgr->ScreenForRect(*aX, *aY, width, height,
                            getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailRect(&left, &top, &width, &height);
      screenRect.xLeft = left;
      screenRect.xRight = left+width;
      screenRect.yTop = top;
      screenRect.yBottom = top+height;
      doConstrain = PR_TRUE;
    }
  }

#define kWindowPositionSlop 100

  if (doConstrain) {
    if (aAllowSlop) {
      if (*aX < screenRect.xLeft - mBounds.width + kWindowPositionSlop)
        *aX = screenRect.xLeft - mBounds.width + kWindowPositionSlop;
      else if (*aX >= screenRect.xRight - kWindowPositionSlop)
        *aX = screenRect.xRight - kWindowPositionSlop;
  
      if (*aY < screenRect.yTop)
        *aY = screenRect.yTop;
      else if (*aY >= screenRect.yBottom - kWindowPositionSlop)
        *aY = screenRect.yBottom - kWindowPositionSlop;
  
    } else {
  
      if (*aX < screenRect.xLeft)
        *aX = screenRect.xLeft;
      else if (*aX >= screenRect.xRight - mBounds.width)
        *aX = screenRect.xRight - mBounds.width;
  
      if (*aY < screenRect.yTop)
        *aY = screenRect.yTop;
      else if (*aY >= screenRect.yBottom - mBounds.height)
        *aY = screenRect.yBottom - mBounds.height;
    }
  }

  return NS_OK;
}






NS_METHOD nsWindow::Move(PRInt32 aX, PRInt32 aY)
{
  if (mWindowType == eWindowType_toplevel ||
      mWindowType == eWindowType_dialog) {
    SetSizeMode(nsSizeMode_Normal);
  }
  Resize(aX, aY, mBounds.width, mBounds.height, PR_FALSE);
  return NS_OK;
}






NS_METHOD nsWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
   Resize( mBounds.x, mBounds.y, aWidth, aHeight, aRepaint);
   return NS_OK;
}






NS_METHOD nsWindow::Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 w,
                      PRInt32 h,
                      PRBool   aRepaint)
{
   
   
   
   if( !mWnd || mWindowType == eWindowType_child ||
       mWindowType == eWindowType_plugin ) 
   {
      
      mBounds.x      = aX ;
      mBounds.y      = aY ;
      mBounds.width  = w  ;
      mBounds.height = h  ;
   }

   
   if( mWnd)
   {
      
      
      POINTL ptl = { aX, aY };
      NS2PM_PARENT( ptl);
      
      ptl.y -= h - 1;
      if( mParent && mWindowType != eWindowType_popup)
      {
         WinMapWindowPoints( mParent->mWnd, WinQueryWindow(mWnd, QW_PARENT), &ptl, 1);
      }
      else if (mWindowType == eWindowType_popup ) {
         
         ptl.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - h - 1 - aY;
      }

      if (!WinSetWindowPos(GetMainWindow(), 0, ptl.x, ptl.y, w, h,
                           SWP_MOVE | SWP_SIZE)) {
         if (aRepaint && mWnd) {
            WinInvalidateRect( mWnd, 0, FALSE);
         }
      }

#if DEBUG_sobotka
      printf("+++++++++++Resized 0x%lx at %ld, %ld to %d x %d (%d,%d)\n",
             mWnd, ptl.x, ptl.y, w, h, aX, aY);
#endif

   }
   return NS_OK;
}






NS_METHOD nsWindow::Enable(PRBool bState)
{
   if (mWnd) {
      WinEnableWindow( GetMainWindow(), !!bState);
   }
   return NS_OK;
}


NS_METHOD nsWindow::IsEnabled(PRBool *aState)
{
   NS_ENSURE_ARG_POINTER(aState);
   *aState = !mWnd || ::WinIsWindowEnabled(mWnd);
   return NS_OK;
}








NS_METHOD nsWindow::SetFocus(PRBool aRaise)
{
    if (mWnd) {
        if (!mInSetFocus) {
           DEBUGFOCUS(SetFocus);
           mInSetFocus = TRUE;
           WinSetFocus( HWND_DESKTOP, mWnd);
           mInSetFocus = FALSE;
        }

    }
    return NS_OK;
}






NS_METHOD nsWindow::GetBounds(nsIntRect &aRect)
{
  if (mFrameWnd) {
    SWP swp;
    WinQueryWindowPos(mFrameWnd, &swp);
    aRect.width = swp.cx;
    aRect.height = swp.cy;
    aRect.x = swp.x;
    aRect.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - (swp.y+swp.cy);
  } else {
    aRect = mBounds;
  }

  return NS_OK;
}






NS_METHOD nsWindow::GetClientBounds(nsIntRect &aRect)
{

   
   aRect.x = 0;
   aRect.y = 0;
   aRect.width = mBounds.width;
   aRect.height = mBounds.height;
   return NS_OK;
}



void nsWindow::GetNonClientBounds(nsIntRect &aRect)
{
  if (mWnd) {
      RECTL r;
      WinQueryWindowRect(mWnd, &r);

      
      aRect.width = r.xRight - r.xLeft;
      aRect.height = r.yBottom - r.yTop;

      
      HWND parent = WinQueryWindow(mWnd, QW_PARENT);
      if (parent) {
        RECTL pr;
        WinQueryWindowRect(parent, &pr);
        r.xLeft -= pr.xLeft;
        r.yTop -= pr.yTop;
      }
      aRect.x = r.xLeft;
      aRect.y = r.yTop;
  } else {
      aRect.SetRect(0,0,0,0);
  }
}







NS_METHOD nsWindow::SetCursor(nsCursor aCursor)
{
  HPOINTER newPointer = NULLHANDLE;

  switch (aCursor) {
    case eCursor_select:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_TEXT, FALSE);
      break;
      
    case eCursor_wait:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);
      break;
  
    case eCursor_hyperlink:
      newPointer = gPtrArray[IDC_SELECTANCHOR-IDC_BASE];
      break;
  
    case eCursor_standard:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE);
      break;
  
    case eCursor_n_resize:
    case eCursor_s_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENS, FALSE);
      break;
  
    case eCursor_w_resize:
    case eCursor_e_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZEWE, FALSE);
      break;
  
    case eCursor_nw_resize:
    case eCursor_se_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENWSE, FALSE);
      break;
  
    case eCursor_ne_resize:
    case eCursor_sw_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENESW, FALSE);
      break;
  
    case eCursor_crosshair:
      newPointer = gPtrArray[IDC_CROSS-IDC_BASE];
      break;
               
    case eCursor_move:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_MOVE, FALSE);
      break;
  
    case eCursor_help:
      newPointer = gPtrArray[IDC_HELP-IDC_BASE];
      break;
  
    case eCursor_copy: 
      newPointer = gPtrArray[IDC_COPY-IDC_BASE];
      break;
  
    case eCursor_alias:
      newPointer = gPtrArray[IDC_ALIAS-IDC_BASE];
      break;
  
    case eCursor_cell:
      newPointer = gPtrArray[IDC_CELL-IDC_BASE];
      break;
  
    case eCursor_grab:
      newPointer = gPtrArray[IDC_GRAB-IDC_BASE];
      break;
  
    case eCursor_grabbing:
      newPointer = gPtrArray[IDC_GRABBING-IDC_BASE];
      break;
  
    case eCursor_spinning:
      newPointer = gPtrArray[IDC_ARROWWAIT-IDC_BASE];
      break;
  
    case eCursor_context_menu:
      
      break;
  
    case eCursor_zoom_in:
      newPointer = gPtrArray[IDC_ZOOMIN-IDC_BASE];
      break;
  
    case eCursor_zoom_out:
      newPointer = gPtrArray[IDC_ZOOMOUT-IDC_BASE];
      break;
  
    case eCursor_not_allowed:
    case eCursor_no_drop:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_ILLEGAL, FALSE);
      break;
  
    case eCursor_col_resize:
      newPointer = gPtrArray[IDC_COLRESIZE-IDC_BASE];
      break;
  
    case eCursor_row_resize:
      newPointer = gPtrArray[IDC_ROWRESIZE-IDC_BASE];
      break;
  
    case eCursor_vertical_text:
      newPointer = gPtrArray[IDC_VERTICALTEXT-IDC_BASE];
      break;
  
    case eCursor_all_scroll:
      
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_MOVE, FALSE);
      break;
  
    case eCursor_nesw_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENESW, FALSE);
      break;
  
    case eCursor_nwse_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENWSE, FALSE);
      break;
  
    case eCursor_ns_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZENS, FALSE);
      break;
  
    case eCursor_ew_resize:
      newPointer = ::WinQuerySysPointer(HWND_DESKTOP, SPTR_SIZEWE, FALSE);
      break;
  
    case eCursor_none:
      newPointer = gPtrArray[IDC_NONE-IDC_BASE];
      break;
  
    default:
      NS_ERROR("Invalid cursor type");
      break;
  }

  if (newPointer) {
    WinSetPointer(HWND_DESKTOP, newPointer);
  }

  return NS_OK;
}








NS_IMETHODIMP nsWindow::SetCursor(imgIContainer* aCursor,
                                  PRUint32 aHotspotX, PRUint32 aHotspotY)
{

  
  
  
  if (mCssCursorImg == aCursor && mCssCursorHPtr) {
    WinSetPointer(HWND_DESKTOP, mCssCursorHPtr);
    return NS_OK;
  }

  nsRefPtr<gfxImageSurface> frame;
  aCursor->CopyFrame(imgIContainer::FRAME_CURRENT,
                     imgIContainer::FLAG_SYNC_DECODE,
                     getter_AddRefs(frame));
  if (!frame)
    return NS_ERROR_NOT_AVAILABLE;

  
  
  PRInt32 width = frame->Width();
  PRInt32 height = frame->Height();
  if (width > 128 || height > 128)
    return NS_ERROR_FAILURE;

  PRUint8* data = frame->Data();

  
  HBITMAP hBmp = CreateBitmapRGB(data, width, height);
  if (!hBmp)
    return NS_ERROR_FAILURE;

  
  HBITMAP hAlpha = CreateTransparencyMask(frame->Format(), data, width, height);
  if (!hAlpha) {
    GpiDeleteBitmap(hBmp);
    return NS_ERROR_FAILURE;
  }

  POINTERINFO info = {0};
  info.fPointer = TRUE;
  info.xHotspot = aHotspotX;
  info.yHotspot = height - aHotspotY - 1;
  info.hbmPointer = hAlpha;
  info.hbmColor = hBmp;

  
  HPOINTER cursor = WinCreatePointerIndirect(HWND_DESKTOP, &info);
  GpiDeleteBitmap(hBmp);
  GpiDeleteBitmap(hAlpha);
  if (cursor == NULL)
    return NS_ERROR_FAILURE;

  
  WinSetPointer(HWND_DESKTOP, cursor);

  
  
  if (mCssCursorHPtr)
    WinDestroyPointer(mCssCursorHPtr);

  
  mCssCursorHPtr = cursor;
  mCssCursorImg = aCursor;

  return NS_OK;
}






#define ALIGNEDBPR(cx,bits) ( ( ( ((cx)*(bits)) + 31) / 32) * 4)

HBITMAP nsWindow::DataToBitmap(PRUint8* aImageData, PRUint32 aWidth,
                               PRUint32 aHeight, PRUint32 aDepth)
{
  
  HPS hps = (HPS)GetNativeData(NS_NATIVE_GRAPHIC);
  if (!hps)
    return 0;

  
  
  struct {
    BITMAPINFOHEADER2 head;
    RGB2 black;
    RGB2 white;
  } bi;

  memset( &bi, 0, sizeof(bi));
  bi.white.bBlue = (BYTE)255;
  bi.white.bGreen = (BYTE)255;
  bi.white.bRed = (BYTE)255;

  
  bi.head.cbFix = sizeof(bi.head);
  bi.head.cx = aWidth;
  bi.head.cy = aHeight;
  bi.head.cPlanes = 1;
  bi.head.cBitCount = aDepth;
  bi.head.ulCompression = BCA_UNCOMP;
  bi.head.cbImage = ALIGNEDBPR(aWidth, aDepth) * aHeight;
  bi.head.cclrUsed = (aDepth == 1 ? 2 : 0);

  
  HBITMAP hBmp = GpiCreateBitmap(hps, &bi.head, CBM_INIT,
                 reinterpret_cast<BYTE*>(aImageData),
                 (BITMAPINFO2*)&bi);

  
  FreeNativeData((void*)hps, NS_NATIVE_GRAPHIC);
  return hBmp;
}





HBITMAP nsWindow::CreateBitmapRGB(PRUint8* aImageData,
                                  PRUint32 aWidth,
                                  PRUint32 aHeight)
{
  
  const PRUint32 bpr = ALIGNEDBPR(aWidth, 24);
  PRUint8* bmp = (PRUint8*)malloc(bpr * aHeight);
  if (!bmp)
    return NULL;

  PRUint32* pSrc = (PRUint32*)aImageData;
  for (PRUint32 row = aHeight; row > 0; --row) {
    PRUint8* pDst = bmp + bpr * (row - 1);

    for (PRUint32 col = aWidth; col > 0; --col) {
      
      
      PRUint32 color = *pSrc++;
      *pDst++ = color;       
      *pDst++ = color >> 8;  
      *pDst++ = color >> 16; 
    }
  }

  
  HBITMAP hAlpha = DataToBitmap(bmp, aWidth, aHeight, 24);

  
  free(bmp);
  return hAlpha;
}



HBITMAP nsWindow::CreateTransparencyMask(gfxASurface::gfxImageFormat format,
                                         PRUint8* aImageData,
                                         PRUint32 aWidth,
                                         PRUint32 aHeight)
{
  
  PRUint32 abpr = ALIGNEDBPR(aWidth, 1);
  PRUint32 cbData = abpr * aHeight;

  
  PRUint8* mono = (PRUint8*)calloc(cbData, 2);
  if (!mono)
    return NULL;

  if (format == gfxASurface::ImageFormatARGB32) {
    
    

    
    PRInt32* pSrc = (PRInt32*)aImageData;
    for (PRUint32 row = aHeight; row > 0; --row) {
      
      PRUint8* pDst = mono + cbData + abpr * (row - 1);
      PRUint8 mask = 0x80;
      for (PRUint32 col = aWidth; col > 0; --col) {
        
        
        if (*pSrc++ >= 0) {
          *pDst |= mask;
        }

        mask >>= 1;
        if (!mask) {
          pDst++;
          mask = 0x80;
        }
      }
    }
  }

  
  HBITMAP hAlpha = DataToBitmap(mono, aWidth, aHeight * 2, 1);

  
  free(mono);
  return hAlpha;
}



NS_IMETHODIMP nsWindow::HideWindowChrome(PRBool aShouldHide) 
{
  HWND hwndFrame = NULLHANDLE;
  HWND hwndTitleBar = NULLHANDLE;
  HWND hwndSysMenu = NULLHANDLE;
  HWND hwndMinMax = NULLHANDLE;
  HWND hwndParent;
  ULONG ulStyle;
  char className[19];

  HWND hwnd = (HWND)GetNativeData(NS_NATIVE_WINDOW);
  for ( ; hwnd != NULLHANDLE; hwnd = WinQueryWindow(hwnd, QW_PARENT)) {
    ::WinQueryClassName(hwnd, 19, className);
    if (strcmp(className, WC_FRAME_STRING) == 0)
    {
      hwndFrame = hwnd;
      break;
    }
  }


  if (aShouldHide) {
    hwndParent = HWND_OBJECT;
    mChromeHidden = TRUE;
  } else {
    hwndParent = hwndFrame;
    mChromeHidden = FALSE;
  }
  hwndTitleBar = (HWND)WinQueryProperty(hwndFrame, "hwndTitleBar");
  if (hwndTitleBar)
    WinSetParent(hwndTitleBar, hwndParent, TRUE);
  hwndSysMenu = (HWND)WinQueryProperty(hwndFrame, "hwndSysMenu");
  if (hwndSysMenu)
    WinSetParent(hwndSysMenu, hwndParent, TRUE);
  hwndMinMax = (HWND)WinQueryProperty(hwndFrame, "hwndMinMax");
  if (hwndMinMax)
    WinSetParent(hwndMinMax, hwndParent, TRUE);
  if (aShouldHide) {
    ulStyle = (ULONG)WinQueryWindowULong(hwndFrame, QWL_STYLE);
    WinSetWindowULong(hwndFrame, QWL_STYLE, ulStyle & ~FS_SIZEBORDER);
    WinSetProperty(hwndFrame, "ulStyle", (PVOID)ulStyle, 0);
    WinSendMsg(hwndFrame, WM_UPDATEFRAME, 0, 0);
  } else {
    ulStyle = (ULONG)WinQueryProperty(hwndFrame, "ulStyle");
    WinSetWindowULong(hwndFrame, QWL_STYLE, ulStyle);
    WinSendMsg(hwndFrame, WM_UPDATEFRAME, MPFROMLONG(FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX), 0);
  }

  return NS_OK;
}






NS_METHOD nsWindow::Invalidate(const nsIntRect &aRect, PRBool aIsSynchronous)
{
  if (mWnd)
  {
    RECTL rcl = { aRect.x, aRect.y, aRect.x + aRect.width, aRect.y + aRect.height };
    NS2PM( rcl);

    WinInvalidateRect( mWnd, &rcl, FALSE);
#if 0
    if( PR_TRUE == aIsSynchronous) {
      Update();
    }
#endif
  }
  return NS_OK;
}






NS_IMETHODIMP nsWindow::Update()
{
  if (mWnd)
    WinUpdateWindow( mWnd);
  return NS_OK;
}






void* nsWindow::GetNativeData(PRUint32 aDataType)
{
    switch(aDataType) {
        case NS_NATIVE_WIDGET:
        case NS_NATIVE_WINDOW:
        case NS_NATIVE_PLUGIN_PORT:
            return (void*)mWnd;

        case NS_NATIVE_GRAPHIC: {
        
        
            HPS hps = 0;
            CheckDragStatus(ACTION_DRAW, &hps);
            if (!hps)
              hps = WinGetPS(mWnd);
            return (void*)hps;
        }

        default: 
            break;
    }

    return NULL;
}

void nsWindow::FreeNativeData(void * data, PRUint32 aDataType)
{
  switch(aDataType)
  {
    case NS_NATIVE_GRAPHIC:
      if (data) {
        if (!ReleaseIfDragHPS((HPS)data))
          WinReleasePS((HPS)data);
      }
      break;
    case NS_NATIVE_WIDGET:
    case NS_NATIVE_WINDOW:
    case NS_NATIVE_PLUGIN_PORT:
      break;
    default: 
      break;
  }
}










nsresult
nsWindow::ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
{
  for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
    const Configuration& configuration = aConfigurations[i];
    nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
    NS_ASSERTION(w->GetParent() == this,
                 "Configured widget is not a child");
    w->SetPluginClipRegion(configuration);
  }
  return NS_OK;
}












void nsWindow::SetPluginClipRegion(const Configuration& aConfiguration)
{
  NS_ASSERTION((mParent && mParent->mWnd), "Child window has no parent");

  
  if (!StoreWindowClipRegion(aConfiguration.mClipRegion) &&
      mBounds == aConfiguration.mBounds) {
    return;
  }

  
  
  mBounds.MoveTo(aConfiguration.mBounds.TopLeft());

  
  HWND hClip = GetPluginClipWindow(mParent->mWnd);
  NS_ASSERTION(hClip, "No clipping window for plugin");
  if (!hClip) {
    return;
  }

  
  const nsTArray<nsIntRect>& rects = aConfiguration.mClipRegion;
  nsIntRect r;
  for (PRUint32 i = 0; i < rects.Length(); ++i) {
    r.UnionRect(r, rects[i]);
  }

  
  SWP    swp;
  POINTL ptl;
  WinQueryWindowPos(hClip, &swp);
  ptl.x = aConfiguration.mBounds.x + r.x;
  ptl.y = mParent->mBounds.height
          - (aConfiguration.mBounds.y + r.y + r.height);

  ULONG  clipFlags = 0;
  if (swp.x != ptl.x || swp.y != ptl.y) {
    clipFlags |= SWP_MOVE;
  }
  if (swp.cx != r.width || swp.cy != r.height) {
    clipFlags |= SWP_SIZE;
  }
  if (clipFlags) {
    WinSetWindowPos(hClip, 0, ptl.x, ptl.y, r.width, r.height, clipFlags);
  }

  
  
  
  WinQueryWindowPos(mWnd, &swp);
  ptl.x = -r.x;
  ptl.y = r.height + r.y - aConfiguration.mBounds.height;

  ULONG  wndFlags = 0;
  if (swp.x != ptl.x || swp.y != ptl.y) {
    wndFlags |= SWP_MOVE;
  }
  if (mBounds.Size() != aConfiguration.mBounds.Size()) {
    wndFlags |= SWP_SIZE;
  }
  if (wndFlags) {
    WinSetWindowPos(mWnd, 0, ptl.x, ptl.y,
                    aConfiguration.mBounds.width,
                    aConfiguration.mBounds.height, wndFlags);
  }

  
  
  if (wndFlags & SWP_SIZE) {
    HWND hChild = WinQueryWindow(mWnd, QW_TOP);
    if (hChild) {
      WinSetWindowPos(hChild, 0, 0, 0, 
                      aConfiguration.mBounds.width,
                      aConfiguration.mBounds.height,
                      SWP_MOVE | SWP_SIZE);
    }
  }

  
  
  if (clipFlags & SWP_SIZE) {
    WinInvalidateRect(mWnd, 0, TRUE);
    WinUpdateWindow(mWnd);
  }
}







HWND nsWindow::GetPluginClipWindow(HWND aParentWnd)
{
  static PRBool registered = FALSE;

  if (mClipWnd) {
    return mClipWnd;
  }

  
  if (!registered) {
    registered = WinRegisterClass(0, "nsClipWnd", 0, 0, 4);
    if (!registered) {
      return 0;
    }
  }

  
  mClipWnd = WinCreateWindow(aParentWnd, "nsClipWnd", "",
                             WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                             0, 0, 0, 0, 0, mWnd, 0, 0, 0);
  if (mClipWnd) {
    if (!WinSetParent(mWnd, mClipWnd, FALSE)) {
      WinDestroyWindow(mClipWnd);
      mClipWnd = 0;
    }
  }

  return mClipWnd;
}







static PRBool
ClipRegionContainedInRect(const nsTArray<nsIntRect>& aClipRects,
                          const nsIntRect& aRect)
{
  for (PRUint32 i = 0; i < aClipRects.Length(); ++i) {
    if (!aRect.Contains(aClipRects[i])) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

void
nsWindow::Scroll(const nsIntPoint& aDelta,
                 const nsTArray<nsIntRect>& aDestRects,
                 const nsTArray<Configuration>& aConfigurations)
{
  
  
  nsTHashtable<nsPtrHashKey<nsWindow> > scrolledWidgets;
  scrolledWidgets.Init();
  for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
    const Configuration& configuration = aConfigurations[i];
    nsWindow* w = static_cast<nsWindow*>(configuration.mChild);
    NS_ASSERTION(w->GetParent() == this,
                 "Configured widget is not a child");
    if (configuration.mBounds == w->mBounds + aDelta) {
      scrolledWidgets.PutEntry(w);
    }
  }

  
  
  
  HPS hps = 0;
  CheckDragStatus(ACTION_SCROLL, &hps);

  
  for (BlitRectIter iter(aDelta, aDestRects); !iter.IsDone(); ++iter) {
    nsIntRect affectedRect;
    affectedRect.UnionRect(iter.Rect(), iter.Rect() - aDelta);

    ULONG flags = SW_INVALIDATERGN;

    
    for (nsWindow* w = static_cast<nsWindow*>(GetFirstChild()); w;
         w = static_cast<nsWindow*>(w->GetNextSibling())) {

      
      
      
      if (w->mBounds.Intersects(affectedRect)) {
        nsPtrHashKey<nsWindow>* entry = scrolledWidgets.GetEntry(w);

        
        
        
        if (entry) {
          flags |= SW_SCROLLCHILDREN;
          scrolledWidgets.RawRemoveEntry(entry);
        } else {
          
          
          
          
          flags &= ~SW_SCROLLCHILDREN;
          break;
        }
      }
    }

    if (flags & SW_SCROLLCHILDREN) {
      for (PRUint32 i = 0; i < aConfigurations.Length(); ++i) {
        const Configuration& configuration = aConfigurations[i];
        nsWindow* w = static_cast<nsWindow*>(configuration.mChild);

        
        
        
        
        
        if (w->mBounds.Intersects(affectedRect)) {
          if (!ClipRegionContainedInRect(configuration.mClipRegion,
                                         affectedRect - (w->mBounds.TopLeft()
                                                         + aDelta)) && mWnd) {
            WinInvalidateRect( mWnd, 0, FALSE);
          }

          
          
          
          
          HWND hPlugin = WinQueryWindow(w->mWnd, QW_TOP);
          if (hPlugin) {
            WinSendMsg(hPlugin, WM_VRNDISABLED, 0, 0);
          }
        }
      }
    }

    
    
    RECTL clip;
    clip.xLeft   = affectedRect.x;
    clip.xRight  = affectedRect.x + affectedRect.width;
    clip.yTop    = mBounds.height - affectedRect.y;
    clip.yBottom = clip.yTop - affectedRect.height;

    WinScrollWindow(mWnd, aDelta.x, -aDelta.y, &clip, &clip, NULL, NULL, flags);
    Update();
  }

  
  
  
  
  ConfigureChildren(aConfigurations);

  if (hps)
    ReleaseIfDragHPS(hps);
}










PRBool nsWindow::OnKey(MPARAM mp1, MPARAM mp2)
{
  nsKeyEvent pressEvent(PR_TRUE, 0, nsnull);
  USHORT fsFlags = SHORT1FROMMP(mp1);
  USHORT usVKey = SHORT2FROMMP(mp2);
  USHORT usChar = SHORT1FROMMP(mp2);
  UCHAR uchScan = CHAR4FROMMP(mp1);

  
  
  
  
  if (fsFlags & KC_VIRTUALKEY && !(fsFlags & KC_KEYUP) &&
      (usVKey == VK_SHIFT || usVKey == VK_CTRL || usVKey == VK_ALTGRAF) ) {
    return PR_FALSE;
  }

  
  
  
  if ((fsFlags & KC_VIRTUALKEY) && (usVKey == VK_ALT) && !usChar && 
      ((fsFlags & KC_LONEKEY) == 0) && (fsFlags & KC_KEYUP)) {
    return PR_FALSE;
  }

   
  if (fsFlags & KC_DEADKEY) {
    
    
    
    
    return PR_TRUE;
  }

  
  
  nsIntPoint point(0,0);
  nsKeyEvent event(PR_TRUE, (fsFlags & KC_KEYUP) ? NS_KEY_UP : NS_KEY_DOWN,
                   this);
  InitEvent(event, &point);
  event.keyCode   = WMChar2KeyCode(mp1, mp2);
  event.isShift   = (fsFlags & KC_SHIFT) ? PR_TRUE : PR_FALSE;
  event.isControl = (fsFlags & KC_CTRL) ? PR_TRUE : PR_FALSE;
  event.isAlt     = (fsFlags & KC_ALT) ? PR_TRUE : PR_FALSE;
  event.isMeta    = PR_FALSE;
  event.charCode = 0;

  
  
  
  
  
  if (((event.keyCode == NS_VK_UP) || (event.keyCode == NS_VK_DOWN)) &&
      !(fsFlags & KC_KEYUP) &&
      ((CHAR3FROMMP(mp1) == 0) || fsFlags & KC_CTRL || fsFlags & KC_ALT) ) {
    if (!(WinGetPhysKeyState(HWND_DESKTOP, uchScan) & 0x8000)) {
      MPARAM mp2;
      if (event.keyCode == NS_VK_UP)
        mp2 = MPFROM2SHORT(0, SB_LINEUP);
      else
        mp2 = MPFROM2SHORT(0, SB_LINEDOWN);
      WinSendMsg(mWnd, WM_VSCROLL, 0, mp2);
      return FALSE;
    }
  }

  pressEvent = event;
  PRBool rc = DispatchWindowEvent(&event);

  
  if (fsFlags & KC_KEYUP) {
    return rc;
  }

  
  
  
  if (fsFlags & KC_INVALIDCOMP) {
    mHaveDeadKey = FALSE;
    
    
    return rc;
  }

  
  pressEvent.message = NS_KEY_PRESS;
  if (rc) { 
    pressEvent.flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }

  if (usChar) {
    USHORT inbuf[2];
    inbuf[0] = usChar;
    inbuf[1] = '\0';

    nsAutoChar16Buffer outbuf;
    PRInt32 bufLength;
    MultiByteToWideChar(0, (const char*)inbuf, 2, outbuf, bufLength);

    pressEvent.charCode = outbuf[0];

    if (pressEvent.isControl && !(fsFlags & (KC_VIRTUALKEY | KC_DEADKEY))) {
      if (!pressEvent.isShift && (pressEvent.charCode >= 'A' && pressEvent.charCode <= 'Z')) {
        pressEvent.charCode = tolower(pressEvent.charCode);
      }
      if (pressEvent.isShift && (pressEvent.charCode >= 'a' && pressEvent.charCode <= 'z')) {
        pressEvent.charCode = toupper(pressEvent.charCode);
      }
      pressEvent.keyCode = 0;
    } else if (!pressEvent.isControl && !pressEvent.isAlt && pressEvent.charCode != 0) {
      if (!(fsFlags & KC_VIRTUALKEY) || 
          ((fsFlags & KC_CHAR) && (pressEvent.keyCode == 0)) ) {
        pressEvent.keyCode = 0;
      } else if (usVKey == VK_SPACE) {
        
      } else if ((fsFlags & KC_VIRTUALKEY) &&
                 isNumPadScanCode(uchScan) && pressEvent.keyCode != 0 && isNumlockOn) {
        
        pressEvent.keyCode = 0;
      } else { 
        pressEvent.charCode = 0;
      }
    }
    rc = DispatchWindowEvent(&pressEvent);
  }

  return rc;
}

void nsWindow::ConstrainZLevel(HWND *aAfter) {

  nsZLevelEvent  event(PR_TRUE, NS_SETZLEVEL, this);
  nsWindow      *aboveWindow = 0;

  InitEvent(event);

  if (*aAfter == HWND_BOTTOM)
    event.mPlacement = nsWindowZBottom;
  else if (*aAfter == HWND_TOP)
    event.mPlacement = nsWindowZTop;
  else {
    event.mPlacement = nsWindowZRelative;
    aboveWindow = GetNSWindowPtr(*aAfter);
  }
  event.mReqBelow = aboveWindow;
  event.mActualBelow = nsnull;

  event.mImmediate = PR_FALSE;
  event.mAdjusted = PR_FALSE;
  DispatchWindowEvent(&event);

  if (event.mAdjusted) {
    if (event.mPlacement == nsWindowZBottom)
      *aAfter = HWND_BOTTOM;
    else if (event.mPlacement == nsWindowZTop)
      *aAfter = HWND_TOP;
    else {
      *aAfter = (HWND)event.mActualBelow->GetNativeData(NS_NATIVE_WINDOW);
      
      if (WinQueryWindowUShort(*aAfter, QWS_ID) == FID_CLIENT) {
        *aAfter = WinQueryWindow(*aAfter, QW_PARENT);
      }
    }
  }
  NS_IF_RELEASE(event.mActualBelow);
}




PRBool nsWindow::ProcessMessage( ULONG msg, MPARAM mp1, MPARAM mp2, MRESULT &rc)
{
    PRBool result = PR_FALSE; 

    switch (msg) {
        case WM_COMMAND: 
        {
           nsMenuEvent event(PR_TRUE, NS_MENU_SELECTED, this);
           event.mCommand = SHORT1FROMMP(mp1);
           InitEvent(event);
           result = DispatchWindowEvent(&event);
        }

        case WM_CONTROL: 
          result = OnControl( mp1, mp2);
          break;

        case WM_CLOSE:  
        case WM_QUIT:   
                        
          mWindowState |= nsWindowState_eClosing;
          DispatchStandardEvent( NS_XUL_CLOSE );
          result = PR_TRUE; 
          break;

        case WM_DESTROY:
            
            OnDestroy();
            result = PR_TRUE;
            break;

        case WM_PAINT:
            result = OnPaint();
            break;

        case WM_TRANSLATEACCEL:
            {
              PQMSG pQmsg = (PQMSG)mp1;
              if (pQmsg->msg == WM_CHAR) 
              {
                LONG mp1 = (LONG)pQmsg->mp1;
                LONG mp2 = (LONG)pQmsg->mp2;

                
                
                
                
                
                
                if (((SHORT1FROMMP(mp1) & (KC_VIRTUALKEY | KC_SHIFT)) &&
                     (SHORT2FROMMP(mp2) == VK_ENTER)) ||
                
                    ((SHORT1FROMMP(mp1) & KC_VIRTUALKEY) &&    
                     ((SHORT1FROMMP(mp1) & (KC_SHIFT | KC_ALT | KC_CTRL)) == 0) &&
                     (SHORT2FROMMP(mp2) == VK_F10)) ||
                
                    ((SHORT1FROMMP(mp1) & KC_VIRTUALKEY) &&    
                     ((SHORT1FROMMP(mp1) & (KC_SHIFT | KC_ALT | KC_CTRL)) == 0) &&
                     (SHORT2FROMMP(mp2) == VK_F1)) ||
                
                    ((SHORT1FROMMP(mp1) & KC_KEYUP) && (SHORT1FROMMP(mp1) & KC_LONEKEY) &&
                     (SHORT2FROMMP(mp2) == VK_ALT)) ||
                
                    ((SHORT1FROMMP(mp1) & (KC_VIRTUALKEY | KC_ALT)) &&
                     (SHORT2FROMMP(mp2) == VK_NEWLINE)) 
                   )
                {
                  return(PR_TRUE);
                }
              }
            }
            break;

        case WM_CHAR:
            result = OnKey( mp1, mp2);
            break;

        case WM_QUERYCONVERTPOS:
          {
            PRECTL pCursorRect = (PRECTL)mp1;
            rc = (MRESULT)QCP_NOCONVERT;
            result = PR_TRUE;
            break;
          }

        
        
        
    
        case WM_BUTTON1DOWN:
          if (!mIsScrollBar)
            WinSetCapture( HWND_DESKTOP, mWnd);
          DispatchMouseEvent( NS_MOUSE_BUTTON_DOWN, mp1, mp2);
            
          gLastButton1Down.x = XFROMMP(mp1);
          gLastButton1Down.y = YFROMMP(mp1);
          break;
        case WM_BUTTON1UP:
          if (!mIsScrollBar)
            WinSetCapture( HWND_DESKTOP, 0); 
          result = DispatchMouseEvent( NS_MOUSE_BUTTON_UP, mp1, mp2);
          break;
        case WM_BUTTON1DBLCLK:
          result = DispatchMouseEvent( NS_MOUSE_DOUBLECLICK, mp1, mp2);
          break;
    
        case WM_BUTTON2DOWN:
          if (!mIsScrollBar)
            WinSetCapture( HWND_DESKTOP, mWnd);
          result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, mp1, mp2, PR_FALSE,
                                      nsMouseEvent::eRightButton);
          break;
        case WM_BUTTON2UP:
          if (!mIsScrollBar)
            WinSetCapture( HWND_DESKTOP, 0); 
          result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, mp1, mp2, PR_FALSE,
                                      nsMouseEvent::eRightButton);
          break;
        case WM_BUTTON2DBLCLK:
          result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, mp1, mp2,
                                      PR_FALSE, nsMouseEvent::eRightButton);
          break;
        case WM_CONTEXTMENU:
          if (SHORT2FROMMP(mp2) == TRUE) {
            HWND hwndCurrFocus = WinQueryFocus(HWND_DESKTOP);
            if (hwndCurrFocus != mWnd) {
              WinSendMsg(hwndCurrFocus, msg, mp1, mp2);
            } else {
              result = DispatchMouseEvent(NS_CONTEXTMENU, mp1, mp2, PR_TRUE,
                                          nsMouseEvent::eLeftButton);
            }
          } else {
            result = DispatchMouseEvent(NS_CONTEXTMENU, mp1, mp2, PR_FALSE,
                                        nsMouseEvent::eRightButton);
          }
          break;

          
          
          
        case WM_CHORD:
          if (WinGetKeyState(HWND_DESKTOP, VK_BUTTON1) & 
              WinGetKeyState(HWND_DESKTOP, VK_BUTTON2) &
              0x8000) {
            PRBool isCopy = PR_FALSE;
            if (abs(XFROMMP(mp1) - gLastButton1Down.x) >
                  (WinQuerySysValue(HWND_DESKTOP, SV_CXMOTIONSTART) / 2) ||
                abs(YFROMMP(mp1) - gLastButton1Down.y) >
                  (WinQuerySysValue(HWND_DESKTOP, SV_CYMOTIONSTART) / 2)) {
              isCopy = PR_TRUE;
            }

            nsKeyEvent event(PR_TRUE, NS_KEY_PRESS, this);
            nsIntPoint point(0,0);
            InitEvent( event, &point);

            event.keyCode   = NS_VK_INSERT;
            if (isCopy) {
              event.isShift   = PR_FALSE;
              event.isControl = PR_TRUE;
            } else {
              event.isShift   = PR_TRUE;
              event.isControl = PR_FALSE;
            }
            event.isAlt     = PR_FALSE;
            event.isMeta    = PR_FALSE;
            event.eventStructType = NS_KEY_EVENT;
            event.charCode = 0;
            
            if (SHORT1FROMMP(mp1) & (KC_VIRTUALKEY|KC_KEYUP|KC_LONEKEY)) {
              USHORT usVKey = SHORT2FROMMP(mp2);
              if (usVKey == VK_SHIFT)
                event.isShift = PR_TRUE;
              if (usVKey == VK_CTRL)
                event.isControl = PR_TRUE;
              if (usVKey == VK_ALTGRAF || usVKey == VK_ALT)
                event.isAlt = PR_TRUE;
            }
            result = DispatchWindowEvent( &event);
          }
          break;

        case WM_BUTTON3DOWN:
          if (!mIsScrollBar)
            WinSetCapture( HWND_DESKTOP, mWnd);
          result = DispatchMouseEvent(NS_MOUSE_BUTTON_DOWN, mp1, mp2, PR_FALSE,
                                      nsMouseEvent::eMiddleButton);
          break;
        case WM_BUTTON3UP:
          if (!mIsScrollBar)
            WinSetCapture( HWND_DESKTOP, 0); 
          result = DispatchMouseEvent(NS_MOUSE_BUTTON_UP, mp1, mp2, PR_FALSE,
                                      nsMouseEvent::eMiddleButton);
          break;
        case WM_BUTTON3DBLCLK:
          result = DispatchMouseEvent(NS_MOUSE_DOUBLECLICK, mp1, mp2, PR_FALSE,
                                      nsMouseEvent::eMiddleButton);
          break;
    
        case WM_MOUSEMOVE:
          {
            static POINTL ptlLastPos = { -1, -1 };
            
            if ( ptlLastPos.x == (SHORT)SHORT1FROMMP(mp1) &&
                 ptlLastPos.y == (SHORT)SHORT2FROMMP(mp1)) {
                return PR_TRUE;
            } else {
                
                ptlLastPos.x = (SHORT)SHORT1FROMMP(mp1);
                ptlLastPos.y = (SHORT)SHORT2FROMMP(mp1);
            }
          }
          result = DispatchMouseEvent( NS_MOUSE_MOVE, mp1, mp2);
          
          if (!mIsScrollBar)
            result = PR_TRUE;
          break;
        case WM_MOUSEENTER:
          result = DispatchMouseEvent( NS_MOUSE_ENTER, mp1, mp2);
          break;
        case WM_MOUSELEAVE:
          result = DispatchMouseEvent( NS_MOUSE_EXIT, mp1, mp2);
          break;

        case WM_APPCOMMAND:
        {
          PRUint32 appCommand = GET_APPCOMMAND_LPARAM(mp2);

          switch (appCommand)
          {
            case APPCOMMAND_BROWSER_BACKWARD:
            case APPCOMMAND_BROWSER_FORWARD:
            case APPCOMMAND_BROWSER_REFRESH:
            case APPCOMMAND_BROWSER_STOP:
              DispatchCommandEvent(appCommand);
              
              rc = (MRESULT)1;
              result = PR_TRUE;
              break;

            default:
              rc = (MRESULT)0;
              result = PR_FALSE;
              break;
          }
          break;
        }
    
        case WM_HSCROLL:
        case WM_VSCROLL:
          result = OnScroll( msg, mp1, mp2);
          break;

        
        
        

        
        
        
        case WM_FOCUSCHANGED:
          DEBUGFOCUS(WM_FOCUSCHANGED);
          if (SHORT1FROMMP(mp2)) {
            ActivateTopLevelWidget();
            ActivatePlugin(HWNDFROMMP(mp1));
          }
          break;

        case WM_WINDOWPOSCHANGED: 
          result = OnReposition( (PSWP) mp1);
          break;

        case WM_PRESPARAMCHANGED:
          
          rc = GetPrevWP()( mWnd, msg, mp1, mp2);
          OnPresParamChanged( mp1, mp2);
          result = PR_TRUE;
          break;

          
        case DM_DRAGOVER:
        case DM_DRAGLEAVE:
        case DM_DROP:
        case DM_RENDERCOMPLETE:
        case DM_DROPHELP:
          OnDragDropMsg(msg, mp1, mp2, rc);
          result = PR_TRUE;
          break;
    }
    
    return result;
}







void    nsWindow::ActivateTopLevelWidget()
{
  nsWindow * top = static_cast<nsWindow*>(GetTopLevelWidget());
  if (top) {
    top->ActivateTopLevelWidget();
  }
  return;
}






void    nsWindow::ActivatePlugin(HWND aWnd)
{
  
  static PRBool inPluginActivate = FALSE;
  if (inPluginActivate) {
    return;
  }

  
  
  if (!WinQueryProperty(mWnd, NS_PLUGIN_WINDOW_PROPERTY_ASSOCIATION)) {
    return;
  }

  
  inPluginActivate = TRUE;
  DEBUGFOCUS(NS_PLUGIN_ACTIVATE);
  DispatchFocus(NS_PLUGIN_ACTIVATE);

  
  
  
  
  
  
  HWND hFocus = 0;
  if (WinIsChild(aWnd, mWnd)) {
    hFocus = aWnd;
  } else {
    hFocus = WinQueryWindow(mWnd, QW_TOP);
    if (hFocus) {
      PID pidFocus, pidThis;
      TID tid;
      WinQueryWindowProcess(hFocus, &pidFocus, &tid);
      WinQueryWindowProcess(mWnd, &pidThis, &tid);
      if (pidFocus != pidThis) {
        hFocus = 0;
      }
    }
  }
  if (hFocus) {
    WinSetFocus(HWND_DESKTOP, hFocus);
  }

  inPluginActivate = FALSE;
  return;
}






void nsWindow::SubclassWindow(BOOL bState)
{
  if (NULL != mWnd) {
    NS_PRECONDITION(::WinIsWindow(0, mWnd), "Invalid window handle");
    
    if (bState) {
        
        mPrevWndProc = WinSubclassWindow(mWnd, fnwpNSWindow);
        NS_ASSERTION(mPrevWndProc, "Null standard window procedure");
        
        SetNSWindowPtr(mWnd, this);
    } 
    else {
        WinSubclassWindow(mWnd, mPrevWndProc);
        SetNSWindowPtr(mWnd, NULL);
        mPrevWndProc = NULL;
    }
  }
}



PRBool nsWindow::OnReposition( PSWP pSwp)
{
   PRBool result = PR_FALSE;
 
   if( pSwp->fl & SWP_MOVE && !(pSwp->fl & SWP_MINIMIZE))
   {
      
      POINTL ptl = { pSwp->x, pSwp->y + pSwp->cy - 1 };
      WinMapWindowPoints( WinQueryWindow( mWnd, QW_PARENT), GetParentHWND(), &ptl, 1);
      PM2NS_PARENT( ptl);
      mBounds.x = ptl.x;
      mBounds.y = ptl.y;
 
      WinMapWindowPoints( GetParentHWND(), HWND_DESKTOP, &ptl, 1);
      result = OnMove( ptl.x, ptl.y);
   }
   if( pSwp->fl & SWP_SIZE && !(pSwp->fl & SWP_MINIMIZE))
      result = OnResize( pSwp->cx, pSwp->cy);

   return result;
}

PRBool nsWindow::OnPresParamChanged( MPARAM mp1, MPARAM mp2)
{
   return PR_FALSE;
}





PRBool nsWindow::OnControl( MPARAM mp1, MPARAM mp2)
{
   return PR_TRUE; 
}







void nsWindow::OnDestroy()
{
   mOnDestroyCalled = PR_TRUE;

   SubclassWindow( PR_FALSE);
   mWnd = 0;

   
   nsBaseWidget::OnDestroy();

   
   
   
   
   
   
   
   if( !(nsWindowState_eDoingDelete & mWindowState) )
   {
      AddRef();
      DispatchStandardEvent( NS_DESTROY );
      Release();
   }

   
   mWindowState |= nsWindowState_eDead;
   mWindowState &= ~(nsWindowState_eLive|nsWindowState_ePrecreate|
                     nsWindowState_eInCreate);
}






PRBool nsWindow::OnMove(PRInt32 aX, PRInt32 aY)
{
  
  nsGUIEvent event(PR_TRUE, NS_MOVE, this);
  InitEvent( event);
  event.refPoint.x = aX;
  event.refPoint.y = aY;
  return DispatchWindowEvent(&event);
}






PRBool nsWindow::OnPaint()
{
  PRBool rc = PR_FALSE;
  nsEventStatus eventStatus = nsEventStatus_eIgnore;

#ifdef NS_DEBUG
  HRGN debugPaintFlashRegion = NULL;
  HPS debugPaintFlashPS = NULL;

  if (debug_WantPaintFlashing()) {
    debugPaintFlashPS = ::WinGetPS(mWnd);
    debugPaintFlashRegion = ::GpiCreateRegion(debugPaintFlashPS, 0, NULL);
    ::WinQueryUpdateRegion(mWnd, debugPaintFlashRegion);
  } 
#endif

  if (mContext && mEventCallback) {
    
    RECTL rcl = { 0 };

    
    
    
    HPS hpsDrag = 0;
    CheckDragStatus(ACTION_PAINT, &hpsDrag);
    HPS hPS = WinBeginPaint(mWnd, hpsDrag, &rcl);

    
    if (!WinIsRectEmpty(0, &rcl)) {
      
      if (mEventCallback) {
        nsPaintEvent event(PR_TRUE, NS_PAINT, this);
        InitEvent(event);

        
        nsIntRect rect;
        rect.x = rcl.xLeft;
        rect.y = GetClientHeight() - rcl.yTop;
        rect.width = rcl.xRight - rcl.xLeft;
        rect.height = rcl.yTop - rcl.yBottom;
        event.rect = &rect;
        event.region = nsnull;

#ifdef NS_DEBUG
        debug_DumpPaintEvent(stdout, this, &event, nsCAutoString("noname"),
                             (PRInt32)mWnd);
#endif 

        nsRefPtr<gfxContext> thebesContext = new gfxContext(mThebesSurface);

        nsCOMPtr<nsIRenderingContext> context;
        nsresult rv = mContext->CreateRenderingContextInstance(*getter_AddRefs(context));
        if (NS_FAILED(rv)) {
          NS_WARNING("CreateRenderingContextInstance failed");
          return PR_FALSE;
        }

        rv = context->Init(mContext, thebesContext);
        if (NS_FAILED(rv)) {
          NS_WARNING("context::Init failed");
          return PR_FALSE;
        }

        event.renderingContext = context;
        
        
        for (int i = 0; i < 10; i++) {
          rc = DispatchWindowEvent(&event, eventStatus);
          if (rc) {
            
            break;
          }
        }
        event.renderingContext = nsnull;

        if (rc) {
          
          
          thebesContext->PopGroupToSource();
          thebesContext->SetOperator(gfxContext::OPERATOR_SOURCE);
          thebesContext->Paint();
        }
      } 
      mThebesSurface->Refresh(&rcl, hPS);
    } 

    WinEndPaint(hPS);
    if (hpsDrag) {
      ReleaseIfDragHPS(hpsDrag);
    }
  } 

#ifdef NS_DEBUG
  if (debug_WantPaintFlashing()) {
    
    
    
    if (nsEventStatus_eIgnore != eventStatus) {
      LONG CurMix = ::GpiQueryMix(debugPaintFlashPS);
      ::GpiSetMix(debugPaintFlashPS, FM_INVERT);

      ::GpiPaintRegion(debugPaintFlashPS, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));
      ::GpiPaintRegion(debugPaintFlashPS, debugPaintFlashRegion);
      PR_Sleep(PR_MillisecondsToInterval(30));

      ::GpiSetMix(debugPaintFlashPS, CurMix);
    } 
    ::GpiDestroyRegion(debugPaintFlashPS, debugPaintFlashRegion);
    ::WinReleasePS(debugPaintFlashPS);
  } 
#endif

  return rc;
}







PRBool nsWindow::OnResize(PRInt32 aX, PRInt32 aY)
{
   mBounds.width = aX;
   mBounds.height = aY;

   
   if (!mThebesSurface) {
     
     
     mThebesSurface = new gfxOS2Surface(mWnd);
   }

   mThebesSurface->Resize(gfxIntSize(aX, aY));

   return DispatchResizeEvent( aX, aY);
}

PRBool nsWindow::DispatchResizeEvent( PRInt32 aX, PRInt32 aY)
{
   
   nsSizeEvent event(PR_TRUE, NS_SIZE, this);
   nsIntRect   rect( 0, 0, aX, aY);

   InitEvent( event);
   event.windowSize = &rect;             
   event.mWinWidth = mBounds.width;
   event.mWinHeight = mBounds.height;

   return DispatchWindowEvent(&event);
}






PRBool nsWindow::DispatchMouseEvent(PRUint32 aEventType, MPARAM mp1, MPARAM mp2,
                                    PRBool aIsContextMenuKey, PRInt16 aButton)
{
  PRBool result = PR_FALSE;

  if (nsnull == mEventCallback) {
    return result;
  }

  nsMouseEvent event(PR_TRUE, aEventType, this, nsMouseEvent::eReal,
                     aIsContextMenuKey
                     ? nsMouseEvent::eContextMenuKey
                     : nsMouseEvent::eNormal);
  event.button = aButton;

  
  if( aEventType && aEventType != NS_MOUSE_ENTER && aEventType != NS_MOUSE_EXIT)
  {
    POINTL ptl;
    if (aEventType == NS_CONTEXTMENU && aIsContextMenuKey) {
      WinQueryPointerPos(HWND_DESKTOP, &ptl);
      WinMapWindowPoints( HWND_DESKTOP, mWnd, &ptl, 1 );
    } else {
      ptl.x = (SHORT)SHORT1FROMMP(mp1);
      ptl.y = (SHORT)SHORT2FROMMP(mp1);
    }
    PM2NS(ptl);
    nsIntPoint pt( ptl.x, ptl.y);
    InitEvent( event, &pt);

    USHORT usFlags = SHORT2FROMMP( mp2);
    event.isShift = (usFlags & KC_SHIFT) ? PR_TRUE : PR_FALSE;
    event.isControl = (usFlags & KC_CTRL) ? PR_TRUE : PR_FALSE;
    event.isAlt = (usFlags & KC_ALT) ? PR_TRUE : PR_FALSE;
  }
  else
  {
    InitEvent( event, nsnull);
    event.isShift = WinIsKeyDown( VK_SHIFT);
    event.isControl = WinIsKeyDown( VK_CTRL);
    event.isAlt = WinIsKeyDown( VK_ALT) || WinIsKeyDown( VK_ALTGRAF);
  }

  event.isMeta    = PR_FALSE;

  
  if (aEventType == NS_MOUSE_DOUBLECLICK &&
      (aButton == nsMouseEvent::eLeftButton ||
       aButton == nsMouseEvent::eRightButton)) {
    event.message = NS_MOUSE_BUTTON_DOWN;
    event.button = (aButton == nsMouseEvent::eLeftButton) ?
                   nsMouseEvent::eLeftButton : nsMouseEvent::eRightButton;
    event.clickCount = 2;
  }
  else {
    event.clickCount = 1;
  }

  NPEvent pluginEvent;

  switch (aEventType)
  {
    case NS_MOUSE_BUTTON_DOWN:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_BUTTON1DOWN;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_BUTTON3DOWN;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_BUTTON2DOWN;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_BUTTON_UP:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_BUTTON1UP;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_BUTTON3UP;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_BUTTON2UP;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_DOUBLECLICK:
      switch (aButton) {
        case nsMouseEvent::eLeftButton:
          pluginEvent.event = WM_BUTTON1DBLCLK;
          break;
        case nsMouseEvent::eMiddleButton:
          pluginEvent.event = WM_BUTTON3DBLCLK;
          break;
        case nsMouseEvent::eRightButton:
          pluginEvent.event = WM_BUTTON2DBLCLK;
          break;
        default:
          break;
      }
      break;
    case NS_MOUSE_MOVE:
      pluginEvent.event = WM_MOUSEMOVE;
      break;
    default:
      break;
  }

  pluginEvent.wParam = 0;




  pluginEvent.lParam = MAKELONG(event.refPoint.x, event.refPoint.y);

  event.pluginEvent = (void *)&pluginEvent;

  
  if (nsnull != mEventCallback) {
    return DispatchWindowEvent(&event);
  }

  return result;
}







PRBool nsWindow::DispatchFocus(PRUint32 aEventType)
{
  
  if (mEventCallback) {
    nsGUIEvent event(PR_TRUE, aEventType, this);
    InitEvent(event);

    
    event.refPoint.x = 0;
    event.refPoint.y = 0;

    NPEvent pluginEvent;

    switch (aEventType)
    {
      case NS_ACTIVATE:
        pluginEvent.event = WM_SETFOCUS;
        break;
      case NS_DEACTIVATE:
        pluginEvent.event = WM_FOCUSCHANGED;
        break;
      case NS_PLUGIN_ACTIVATE:
        pluginEvent.event = WM_FOCUSCHANGED;
        break;
      default:
        break;
    }

    event.pluginEvent = (void *)&pluginEvent;
    return DispatchWindowEvent(&event);
  }
  return PR_FALSE;
}







PRBool nsWindow::OnScroll( ULONG msgid, MPARAM mp1, MPARAM mp2)
{
   return( (msgid == WM_HSCROLL) ? OnHScroll(mp1, mp2) : OnVScroll(mp1, mp2) );
}

PRBool nsWindow::OnVScroll( MPARAM mp1, MPARAM mp2)
{
    if (nsnull != mEventCallback) {
        nsMouseScrollEvent scrollEvent(PR_TRUE, NS_MOUSE_SCROLL, this);
        InitEvent(scrollEvent);
        scrollEvent.isShift = WinIsKeyDown( VK_SHIFT);
        scrollEvent.isControl = WinIsKeyDown( VK_CTRL);
        scrollEvent.isAlt = WinIsKeyDown( VK_ALT) || WinIsKeyDown( VK_ALTGRAF);
        scrollEvent.isMeta = PR_FALSE;
        scrollEvent.scrollFlags = nsMouseScrollEvent::kIsVertical;
        switch (SHORT2FROMMP(mp2)) {
          case SB_LINEUP:
            scrollEvent.delta = -1;
            break;
          case SB_LINEDOWN:
            scrollEvent.delta = 1;
            break;
          case SB_PAGEUP:
            scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
            scrollEvent.delta = -1;
            break;
          case SB_PAGEDOWN:
            scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
            scrollEvent.delta = 1;
            break;
          default:
            scrollEvent.delta = 0;
            break;
        }
        DispatchWindowEvent(&scrollEvent);
    }
    return PR_FALSE;
}

PRBool nsWindow::OnHScroll( MPARAM mp1, MPARAM mp2)
{
    if (nsnull != mEventCallback) {
        nsMouseScrollEvent scrollEvent(PR_TRUE, NS_MOUSE_SCROLL, this);
        InitEvent(scrollEvent);
        scrollEvent.isShift = WinIsKeyDown( VK_SHIFT);
        scrollEvent.isControl = WinIsKeyDown( VK_CTRL);
        scrollEvent.isAlt = WinIsKeyDown( VK_ALT) || WinIsKeyDown( VK_ALTGRAF);
        scrollEvent.isMeta = PR_FALSE;
        scrollEvent.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
        switch (SHORT2FROMMP(mp2)) {
          case SB_LINELEFT:
            scrollEvent.delta = -1;
            break;
          case SB_LINERIGHT:
            scrollEvent.delta = 1;
            break;
          case SB_PAGELEFT:
            scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
            scrollEvent.delta = -1;
            break;
          case SB_PAGERIGHT:
            scrollEvent.scrollFlags |= nsMouseScrollEvent::kIsFullPage;
            scrollEvent.delta = 1;
            break;
          default:
            scrollEvent.delta = 0;
            break;
        }
        DispatchWindowEvent(&scrollEvent);
    }
    return PR_FALSE;
}



#define MAX_TITLEBAR_LENGTH 256

NS_METHOD nsWindow::SetTitle(const nsAString& aTitle) 
{
   if (mWnd)
   {
      PRUnichar* uchtemp = ToNewUnicode(aTitle);
      for (PRUint32 i=0;i<aTitle.Length();i++) {
       switch (uchtemp[i]) {
         case 0x2018:
         case 0x2019:
           uchtemp[i] = 0x0027;
           break;
         case 0x201C:
         case 0x201D:
           uchtemp[i] = 0x0022;
           break;
         case 0x2014:
           uchtemp[i] = 0x002D;
           break;
       }
      }

      nsAutoCharBuffer title;
      PRInt32 titleLength;
      WideCharToMultiByte(0, uchtemp, aTitle.Length(), title, titleLength);
      if (titleLength > MAX_TITLEBAR_LENGTH) {
        title[MAX_TITLEBAR_LENGTH] = '\0';
      }
      ::WinSetWindowText(GetMainWindow(), title.Elements());
      if (mChromeHidden) {
         
         if (mFrameWnd) {
            HWND hwndTitleBar = (HWND)::WinQueryProperty(mFrameWnd,
                                                         "hwndTitleBar");
            ::WinSetWindowText(hwndTitleBar, title.Elements());
         }
      }
      nsMemory::Free(uchtemp);
   }
   return NS_OK;
} 




NS_METHOD nsWindow::SetIcon(const nsAString& aIconSpec) 
{
  static HPOINTER hDefaultIcon = 0;
  HPOINTER        hWorkingIcon = 0;

  
  nsCOMPtr<nsILocalFile> iconFile;
  ResolveIconName(aIconSpec, NS_LITERAL_STRING(".ico"),
                  getter_AddRefs(iconFile));

  
  if (iconFile) {
    nsCAutoString path;
    iconFile->GetNativePath(path);

    if (mFrameIcon)
      WinFreeFileIcon(mFrameIcon);

    mFrameIcon = WinLoadFileIcon(path.get(), FALSE);
    hWorkingIcon = mFrameIcon;
  }

  
  
  if (hWorkingIcon == 0) {
    if (hDefaultIcon == 0) {
      hDefaultIcon = WinLoadPointer(HWND_DESKTOP, 0, 1);
      if (hDefaultIcon == 0)
        hDefaultIcon =  WinQuerySysPointer(HWND_DESKTOP, SPTR_APPICON, FALSE);
    }
    hWorkingIcon = hDefaultIcon;
  }

  WinSendMsg(mFrameWnd, WM_SETICON, (MPARAM)hWorkingIcon, (MPARAM)0);
  return NS_OK;
}

PRBool
nsWindow::HasPendingInputEvent()
{
  return (WinQueryQueueStatus(HWND_DESKTOP) & (QS_KEY | QS_MOUSE)) != 0;
}





nsresult nsWindow::GetWindowText( nsString &aStr, PRUint32 *rc)
{
   if( mWnd)
   {
      
      int length = WinQueryWindowTextLength( mWnd);
      char *tmp = new char [ length + 1 ];
      WinQueryWindowText( mWnd, length + 1, tmp);
      aStr.AssignWithConversion( tmp);
      delete [] tmp;
   }
   return NS_OK;
}





void nsWindow::AddToStyle( ULONG style)
{
   if( mWnd)
      WinSetWindowBits( mWnd, QWL_STYLE, style, style);
}

void nsWindow::RemoveFromStyle( ULONG style)
{
   if( mWnd)
   {
      ULONG oldStyle = WinQueryWindowULong( mWnd, QWL_STYLE);
      oldStyle &= ~style;
      WinSetWindowULong( mWnd, QWL_STYLE, oldStyle);
   }
}



NS_IMETHODIMP nsWindow::GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState)
{
  PRUint32  vkey;

  NS_ENSURE_ARG_POINTER(aLEDState);

  switch (aKeyCode) {
    case NS_VK_CAPS_LOCK:
      vkey = VK_CAPSLOCK;
      break;
    case NS_VK_NUM_LOCK:
      vkey = VK_NUMLOCK;
      break;
    case NS_VK_SCROLL_LOCK:
      vkey = VK_SCRLLOCK;
      break;
    default:
      *aLEDState = PR_FALSE;
      return NS_OK;
  }

  *aLEDState = (::WinGetKeyState(HWND_DESKTOP, vkey) & 1) != 0;
  return NS_OK;
}
























PRBool nsWindow::OnDragDropMsg(ULONG msg, MPARAM mp1, MPARAM mp2, MRESULT &mr)
{
  nsresult rv;
  PRUint32 eventType = 0;
  PRUint32 dragFlags = 0;

  mr = 0;
  nsCOMPtr<nsIDragService> dragService =
                    do_GetService("@mozilla.org/widget/dragservice;1", &rv);
  if (dragService) {
    nsCOMPtr<nsIDragSessionOS2> dragSession(
                        do_QueryInterface(dragService, &rv));
    if (dragSession) {

        
      switch (msg) {

        case DM_DRAGOVER:
          dragService->FireDragEventAtSource(NS_DRAGDROP_DRAG);

          rv = dragSession->DragOverMsg((PDRAGINFO)mp1, mr, &dragFlags);
          eventType = NS_DRAGDROP_OVER;
          break;

        case DM_DRAGLEAVE:
          rv = dragSession->DragLeaveMsg((PDRAGINFO)mp1, &dragFlags);
          eventType = NS_DRAGDROP_EXIT;
          break;

        case DM_DROP:
          rv = dragSession->DropMsg((PDRAGINFO)mp1, mWnd, &dragFlags);
          eventType = NS_DRAGDROP_DROP;
          break;

        case DM_DROPHELP:
          rv = dragSession->DropHelpMsg((PDRAGINFO)mp1, &dragFlags);
          eventType = NS_DRAGDROP_EXIT;
          break;

        case DM_RENDERCOMPLETE:
          rv = dragSession->RenderCompleteMsg((PDRAGTRANSFER)mp1,
                                              SHORT1FROMMP(mp2), &dragFlags);
          eventType = NS_DRAGDROP_DROP;
          break;

        default:
          rv = NS_ERROR_FAILURE;
      }

        
      if (NS_SUCCEEDED(rv)) {
        mDragStatus = gDragStatus = (dragFlags & DND_DragStatus);

        if (dragFlags & DND_DispatchEnterEvent)
          DispatchDragDropEvent(NS_DRAGDROP_ENTER);

        if (dragFlags & DND_DispatchEvent)
          DispatchDragDropEvent(eventType);

        if (dragFlags & DND_GetDragoverResult)
          dragSession->GetDragoverResult(mr);

        if (dragFlags & DND_ExitSession)
          dragSession->ExitSession(&dragFlags);
      }
    }
  }
    
  gDragStatus = mDragStatus = (dragFlags & DND_DragStatus);

  return PR_TRUE;
}








PRBool nsWindow::CheckDragStatus(PRUint32 aAction, HPS * oHps)
{
  PRBool rtn    = PR_TRUE;
  PRBool getHps = PR_FALSE;

  switch (aAction) {

      
    case ACTION_PAINT:
    case ACTION_SCROLL:
      if (gDragStatus & DND_MozDrag)
        getHps = PR_TRUE;
      break;

      
    case ACTION_DRAW:
      if ((gDragStatus & DND_MozDrag) ||
          (mDragStatus & DND_NativeDrag))
        getHps = PR_TRUE;
      break;

      
    case ACTION_SHOW:
      if ((gDragStatus & (DND_NativeDrag | DND_InDrop)) == DND_NativeDrag)
        rtn = PR_FALSE;
      break;

      
    case ACTION_PTRPOS:
      if (!gDragStatus)
        rtn = PR_FALSE;
      break;

    default:
      rtn = PR_FALSE;
  }

    
    
    
    
    
  if (oHps) {
    if (getHps && !mDragHps) {
      mDragHps = DrgGetPS(mWnd);
      *oHps = mDragHps;
    } else {
      *oHps = 0;
    }
  }

  return rtn;
}






PRBool nsWindow::ReleaseIfDragHPS(HPS aHps)
{

  if (mDragHps && aHps == mDragHps) {
    DrgReleasePS(mDragHps);
    mDragHps = 0;
    return PR_TRUE;
  }

  return PR_FALSE;
}







PRUint32 WMChar2KeyCode(MPARAM mp1, MPARAM mp2)
{
  PRUint32 rc = SHORT1FROMMP(mp2);  
  PRUint32 rcmask = rc & 0x00FF;    
  USHORT sc = CHAR4FROMMP(mp1);     
  USHORT flags = SHORT1FROMMP(mp1); 

  
  
  
  
  
  if (!(flags & (KC_VIRTUALKEY | KC_DEADKEY)) ||
      (rcmask >= '0' && rcmask <= '9' &&             
       (isNumPadScanCode(sc) ? isNumlockOn : 1)) ) { 
    if (flags & KC_KEYUP) { 
                            
      rc = rcmask;
    } else { 
      if (! (flags & KC_CHAR)) {
        if ((flags & KC_ALT) || (flags & KC_CTRL))
          rc = rcmask;
        else
          rc = 0;
      }
    }

    if (rc < 0xFF) {
      if (rc >= 'a' && rc <= 'z') { 
                                    
        rc = rc - 'a' + NS_VK_A;
      } else if (rc >= 'A' && rc <= 'Z') { 
        rc = rc - 'A' + NS_VK_A;
      } else if (rc >= '0' && rc <= '9') {
        
        rc = rc - '0' + NS_VK_0;
      } else {
        
        
        switch (sc) {
          case 0x02: rc = NS_VK_1;             break;
          case 0x03: rc = NS_VK_2;             break;
          case 0x04: rc = NS_VK_3;             break;
          case 0x05: rc = NS_VK_4;             break;
          case 0x06: rc = NS_VK_5;             break;
          case 0x07: rc = NS_VK_6;             break;
          case 0x08: rc = NS_VK_7;             break;
          case 0x09: rc = NS_VK_8;             break;
          case 0x0A: rc = NS_VK_9;             break;
          case 0x0B: rc = NS_VK_0;             break;
          case 0x0D: rc = NS_VK_EQUALS;        break;
          case 0x1A: rc = NS_VK_OPEN_BRACKET;  break;
          case 0x1B: rc = NS_VK_CLOSE_BRACKET; break;
          case 0x27: rc = NS_VK_SEMICOLON;     break;
          case 0x28: rc = NS_VK_QUOTE;         break;
          case 0x29: rc = NS_VK_BACK_QUOTE;    break;
          case 0x2B: rc = NS_VK_BACK_SLASH;    break;
          case 0x33: rc = NS_VK_COMMA;         break;
          case 0x34: rc = NS_VK_PERIOD;        break;
          case 0x35: rc = NS_VK_SLASH;         break;
          case 0x37: rc = NS_VK_MULTIPLY;      break;
          case 0x4A: rc = NS_VK_SUBTRACT;      break;
          case 0x4C: rc = NS_VK_CLEAR;         break; 
          case 0x4E: rc = NS_VK_ADD;           break;
          case 0x5C: rc = NS_VK_DIVIDE;        break;
          default: break;
        } 
      } 
    } 
  } else if (flags & KC_VIRTUALKEY) {
    USHORT vk = SHORT2FROMMP( mp2);
    if (flags & KC_KEYUP) { 
                            
      rc = rcmask;
    }
    if (isNumPadScanCode(sc) &&
        (((flags & KC_ALT) && (sc != PMSCAN_PADPERIOD)) ||
          ((flags & (KC_CHAR | KC_SHIFT)) == KC_CHAR)  ||
          ((flags & KC_KEYUP) && rc != 0) )) {
      CHAR numpadMap[] = {NS_VK_NUMPAD7, NS_VK_NUMPAD8, NS_VK_NUMPAD9, 0,
                          NS_VK_NUMPAD4, NS_VK_NUMPAD5, NS_VK_NUMPAD6, 0,
                          NS_VK_NUMPAD1, NS_VK_NUMPAD2, NS_VK_NUMPAD3,
                          NS_VK_NUMPAD0, NS_VK_DECIMAL};
      
      
      if (numpadMap[sc - PMSCAN_PAD7] != 0) { 
        if (flags & KC_ALT) 
          rc = 0;
        else
          rc = numpadMap[sc - PMSCAN_PAD7];
      } else {                                
        rc = 0; 
      }
    } else if (!(flags & KC_CHAR) || isNumPadScanCode(sc) ||
               (vk == VK_BACKSPACE) || (vk == VK_TAB) || (vk == VK_BACKTAB) ||
               (vk == VK_ENTER) || (vk == VK_NEWLINE) || (vk == VK_SPACE)) {
      if (vk >= VK_F1 && vk <= VK_F24) {
        rc = NS_VK_F1 + (vk - VK_F1);
      }
      else switch (vk) {
        case VK_NUMLOCK:   rc = NS_VK_NUM_LOCK; break;
        case VK_SCRLLOCK:  rc = NS_VK_SCROLL_LOCK; break;
        case VK_ESC:       rc = NS_VK_ESCAPE; break; 
        case VK_BACKSPACE: rc = NS_VK_BACK; break;
        case VK_TAB:       rc = NS_VK_TAB; break;
        case VK_BACKTAB:   rc = NS_VK_TAB; break; 
        case VK_CLEAR:     rc = NS_VK_CLEAR; break;
        case VK_NEWLINE:   rc = NS_VK_RETURN; break;
        case VK_ENTER:     rc = NS_VK_RETURN; break;
        case VK_SHIFT:     rc = NS_VK_SHIFT; break;
        case VK_CTRL:      rc = NS_VK_CONTROL; break;
        case VK_ALT:       rc = NS_VK_ALT; break;
        case VK_PAUSE:     rc = NS_VK_PAUSE; break;
        case VK_CAPSLOCK:  rc = NS_VK_CAPS_LOCK; break;
        case VK_SPACE:     rc = NS_VK_SPACE; break;
        case VK_PAGEUP:    rc = NS_VK_PAGE_UP; break;
        case VK_PAGEDOWN:  rc = NS_VK_PAGE_DOWN; break;
        case VK_END:       rc = NS_VK_END; break;
        case VK_HOME:      rc = NS_VK_HOME; break;
        case VK_LEFT:      rc = NS_VK_LEFT; break;
        case VK_UP:        rc = NS_VK_UP; break;
        case VK_RIGHT:     rc = NS_VK_RIGHT; break;
        case VK_DOWN:      rc = NS_VK_DOWN; break;
        case VK_PRINTSCRN: rc = NS_VK_PRINTSCREEN; break;
        case VK_INSERT:    rc = NS_VK_INSERT; break;
        case VK_DELETE:    rc = NS_VK_DELETE; break;
      } 
    }
  } 

  return rc;
}

PCSZ nsWindow::WindowClass()
{
    const PCSZ className = "MozillaWindowClass";

    if (!nsWindow::sIsRegistered)
    {
        nsWindow::sIsRegistered = WinRegisterClass((HAB)0, className,
                                                   WinDefWindowProc, 0, 4);
    }

    return className;
}

ULONG nsWindow::WindowStyle()
{
   return BASE_CONTROL_STYLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
}

ULONG nsWindow::GetFCFlags()
{
  ULONG style = FCF_TITLEBAR | FCF_SYSMENU | FCF_TASKLIST |
                FCF_CLOSEBUTTON | FCF_NOBYTEALIGN | FCF_AUTOICON |
                (gIsDBCS ? FCF_DBE_APPSTAT : 0);

  if (mWindowType == eWindowType_dialog) {
    style |= FCF_DIALOGBOX;
    if (mBorderStyle == eBorderStyle_default) {
      style |= FCF_DLGBORDER;
    } else {
      style |= FCF_SIZEBORDER | FCF_MINMAX;
    }
  }
  else {
    style |= FCF_SIZEBORDER | FCF_MINMAX;
  }

  if (mWindowType == eWindowType_invisible) {
    style &= ~FCF_TASKLIST;
  }

  if (mBorderStyle != eBorderStyle_default && mBorderStyle != eBorderStyle_all) {
    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_resizeh)) {
      style &= ~FCF_SIZEBORDER;
      style |= FCF_DLGBORDER;
    }
    
    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_border))
      style &= ~(FCF_DLGBORDER | FCF_SIZEBORDER);
    
    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_title)) {
      style &= ~(FCF_TITLEBAR | FCF_TASKLIST);
    }

    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_close))
      style &= ~FCF_CLOSEBUTTON;

    if (mBorderStyle == eBorderStyle_none ||
      !(mBorderStyle & (eBorderStyle_menu | eBorderStyle_close)))
      style &= ~FCF_SYSMENU;
    
    
    
    
    
    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_minimize))
      style &= ~FCF_MINBUTTON;
    
    if (mBorderStyle == eBorderStyle_none || !(mBorderStyle & eBorderStyle_maximize))
      style &= ~FCF_MAXBUTTON;
  }

  return style;
}
