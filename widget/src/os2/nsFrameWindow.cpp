





















































#include "nsFrameWindow.h"
#include "nsIRollupListener.h"
#include "nsIDeviceContext.h"
#include "nsIComponentManager.h"
#include "nsGfxCIID.h"

extern nsIRollupListener * gRollupListener;
extern nsIWidget         * gRollupWidget;
extern PRBool              gRollupConsumeRollupEvent;

#ifdef DEBUG_FOCUS
  extern int currentWindowIdentifier;
#endif

nsFrameWindow::nsFrameWindow() : nsWindow()
{
   fnwpDefFrame = 0;
   mWindowType  = eWindowType_toplevel;
   mNeedActivation = PR_FALSE;
}

nsFrameWindow::~nsFrameWindow()
{
}

void nsFrameWindow::SetWindowListVisibility( PRBool bState)
{
   HSWITCH hswitch;
   SWCNTRL swctl;

   hswitch = WinQuerySwitchHandle(mFrameWnd, 0);
   if( hswitch)
   {
      WinQuerySwitchEntry( hswitch, &swctl);
      swctl.uchVisibility = bState ? SWL_VISIBLE : SWL_INVISIBLE;
      swctl.fbJump        = bState ? SWL_JUMPABLE : SWL_NOTJUMPABLE;
      WinChangeSwitchEntry( hswitch, &swctl);
   }
}


void nsFrameWindow::RealDoCreate( HWND hwndP, nsWindow *aParent,
                                  const nsIntRect &aRect,
                                  EVENT_CALLBACK aHandleEventFunction,
                                  nsIDeviceContext *aContext,
                                  nsIAppShell *aAppShell,
                                  nsWidgetInitData *aInitData, HWND hwndO)
{
   nsIntRect rect = aRect;
   if( aParent)  
   {
      nsIntRect clientRect;
      aParent->GetBounds(rect);
      aParent->GetClientBounds(clientRect);
      rect.x += aRect.x + clientRect.x;
      rect.y += aRect.y + clientRect.y;
      rect.width = aRect.width;
      rect.height = aRect.height;
      hwndP = aParent->GetMainWindow();
      rect.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - (rect.y + rect.height);
   }
   else          
   {
      rect = aRect;
      rect.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - (aRect.y + aRect.height);
   }

#if DEBUG_sobotka
   printf("\nIn nsFrameWindow::RealDoCreate:\n");
   printf("   hwndP = %lu\n", hwndP);
   printf("   aParent = 0x%lx\n", &aParent);
   printf("   aRect = %ld, %ld, %ld, %ld\n", aRect.x, aRect.y, aRect.height, aRect.width);
#endif

   ULONG fcfFlags = GetFCFlags();

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

#ifdef DEBUG_FOCUS
   mWindowIdentifier = currentWindowIdentifier;
   currentWindowIdentifier++;
   if (aInitData && (aInitData->mWindowType == eWindowType_toplevel))
     DEBUGFOCUS(Create Frame Window);
   else
     DEBUGFOCUS(Create Window);
#endif

   mFrameWnd = WinCreateStdWindow( HWND_DESKTOP,
                                   0,
                                   &fcfFlags,
                                   WindowClass(),
                                   "Title",
                                   style,
                                   NULLHANDLE,
                                   0,
                                   &mWnd);

  
   
   if (hwndP)
     WinSetOwner(mFrameWnd, hwndP);


   
   HWND hwndTitleBar = WinWindowFromID(mFrameWnd, FID_TITLEBAR);
   WinSetProperty(mFrameWnd, "hwndTitleBar", (PVOID)hwndTitleBar, 0);
   HWND hwndSysMenu = WinWindowFromID(mFrameWnd, FID_SYSMENU);
   WinSetProperty(mFrameWnd, "hwndSysMenu", (PVOID)hwndSysMenu, 0);
   HWND hwndMinMax = WinWindowFromID(mFrameWnd, FID_MINMAX);
   WinSetProperty(mFrameWnd, "hwndMinMax", (PVOID)hwndMinMax, 0);


   SetWindowListVisibility( PR_FALSE);  

   NS_ASSERTION( mFrameWnd, "Couldn't create frame");

   
   
   
   nsIntRect frameRect = rect;
   long minheight; 

   if ( fcfFlags & FCF_SIZEBORDER) {
      minheight = 2 * WinQuerySysValue( HWND_DESKTOP, SV_CYSIZEBORDER);
   }
   else if ( fcfFlags & FCF_DLGBORDER) {
      minheight = 2 * WinQuerySysValue( HWND_DESKTOP, SV_CYDLGFRAME);
   }
   else {
      minheight = 2 * WinQuerySysValue( HWND_DESKTOP, SV_CYBORDER);
   }
   if ( fcfFlags & FCF_TITLEBAR) {
      minheight += WinQuerySysValue( HWND_DESKTOP, SV_CYTITLEBAR);
   }
   if ( frameRect.height < minheight) {
      frameRect.height = minheight;
   }

   
   mParent = nsnull;

   
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

   
   
   
   
   mBounds = frameRect;
   mBounds.height = frameRect.height;

   
   mEventCallback = aHandleEventFunction;

   if( mParent)
      mParent->AddChild( this);

   

   DispatchStandardEvent( NS_CREATE );
   SubclassWindow(TRUE);
   PostCreateWidget();

   
   fnwpDefFrame = WinSubclassWindow( mFrameWnd, fnwpFrame);
   WinSetWindowPtr( mFrameWnd, QWL_USER, this);


   WinSetWindowPos(mFrameWnd, 0, frameRect.x, frameRect.y, frameRect.width, frameRect.height, SWP_SIZE | SWP_MOVE);
}


void nsFrameWindow::UpdateClientSize()
{
   RECTL rcl = { 0, 0, mBounds.width, mBounds.height };
   WinCalcFrameRect( mFrameWnd, &rcl, TRUE); 
   mSizeClient.width = rcl.xRight - rcl.xLeft;
   mSizeClient.height = rcl.yTop - rcl.yBottom;
   mSizeBorder.width = (mBounds.width - mSizeClient.width) / 2;
   mSizeBorder.height = (mBounds.height - mSizeClient.height) / 2;
}

nsresult nsFrameWindow::GetClientBounds( nsIntRect &aRect)
{
   RECTL rcl = { 0, 0, mBounds.width, mBounds.height };
   WinCalcFrameRect( mFrameWnd, &rcl, TRUE); 
   aRect.x = rcl.xLeft;
   aRect.y = mBounds.height - rcl.yTop;
   aRect.width = mSizeClient.width;
   aRect.height = mSizeClient.height;
   return NS_OK;
}


PRBool nsFrameWindow::OnReposition( PSWP pSwp)
{
   return PR_TRUE;
}


nsresult nsFrameWindow::Show( PRBool bState)
{
   if( mWnd)
   {
      ULONG ulFlags;
      if( bState) {
         ULONG ulStyle = WinQueryWindowULong( GetMainWindow(), QWL_STYLE);
         ulFlags = SWP_SHOW;
         
         if (WinIsWindowVisible(WinQueryWindow(GetMainWindow(), QW_PARENT)))
           ulFlags |= SWP_ACTIVATE;
         if (!( ulStyle & WS_VISIBLE)) {
            PRInt32 sizeMode;
            GetSizeMode( &sizeMode);
            if ( sizeMode == nsSizeMode_Maximized) {
               ulFlags |= SWP_MAXIMIZE;
            } else if ( sizeMode == nsSizeMode_Minimized) {
               ulFlags |= SWP_MINIMIZE;
            } else {
               ulFlags |= SWP_RESTORE;
            }
         }
         if( ulStyle & WS_MINIMIZED)
            ulFlags |= (SWP_RESTORE | SWP_MAXIMIZE);
      }
      else
         ulFlags = SWP_HIDE | SWP_DEACTIVATE;
      WinSetWindowPos( GetMainWindow(), NULLHANDLE, 0L, 0L, 0L, 0L, ulFlags);
      SetWindowListVisibility( bState);
   }

   return NS_OK;
}






void    nsFrameWindow::ActivateTopLevelWidget()
{
  
  
  

  if (mNeedActivation) {
    PRInt32 sizeMode;
    GetSizeMode(&sizeMode);
    if (sizeMode != nsSizeMode_Minimized) {
      mNeedActivation = PR_FALSE;
      DEBUGFOCUS(NS_ACTIVATE);
      DispatchFocus(NS_ACTIVATE);
    }
  }
  return;
}


MRESULT EXPENTRY fnwpFrame( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   
   if (nsnull != gRollupListener && nsnull != gRollupWidget) {
      if (msg == WM_TRACKFRAME || msg == WM_MINMAXFRAME ||
          msg == WM_BUTTON1DOWN || msg == WM_BUTTON2DOWN || msg == WM_BUTTON3DOWN) {
         
         if (PR_FALSE == nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget)) {
            gRollupListener->Rollup(PR_UINT32_MAX, nsnull);

            
            



         } 
      }
   }

   nsFrameWindow *pFrame = (nsFrameWindow*) WinQueryWindowPtr( hwnd, QWL_USER);
   return pFrame->FrameMessage( msg, mp1, mp2);
}


MRESULT nsFrameWindow::FrameMessage( ULONG msg, MPARAM mp1, MPARAM mp2)
{
   MRESULT mresult = 0;
   BOOL    bDone = FALSE;

   switch (msg)
   {
      case WM_WINDOWPOSCHANGED:
      {
         PSWP pSwp = (PSWP) mp1;

         
         if( pSwp->fl & SWP_MOVE && !(pSwp->fl & SWP_MINIMIZE))
         {
            
            POINTL ptl = { pSwp->x, pSwp->y + pSwp->cy  };
            ptl.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - ptl.y  ;
            mBounds.x = ptl.x;
            mBounds.y = ptl.y;
            OnMove( ptl.x, ptl.y);
         }

         
         if( pSwp->fl & SWP_SIZE && !(pSwp->fl & SWP_MINIMIZE))
         {
            mresult = (*fnwpDefFrame)( mFrameWnd, msg, mp1, mp2);
            bDone = TRUE;

            mBounds.width = pSwp->cx;
            mBounds.height = pSwp->cy;

            UpdateClientSize();
            DispatchResizeEvent( mSizeClient.width, mSizeClient.height);
         }
 
         if (pSwp->fl & (SWP_MAXIMIZE | SWP_MINIMIZE | SWP_RESTORE)) {
           nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);
            if (pSwp->fl & SWP_MAXIMIZE)
              event.mSizeMode = nsSizeMode_Maximized;
            else if (pSwp->fl & SWP_MINIMIZE)
              event.mSizeMode = nsSizeMode_Minimized;
            else
              event.mSizeMode = nsSizeMode_Normal;
            InitEvent(event);
            DispatchWindowEvent(&event);
         }

         break;
      }

      
      
      
      
      

      case WM_ADJUSTWINDOWPOS:
      {
        if (mChromeHidden && ((PSWP)mp1)->fl & SWP_MINIMIZE) {
          HWND hwndTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndMinMax");
          if (hwndTemp)
            WinSetParent(hwndTemp, mFrameWnd, TRUE);
          hwndTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndTitleBar");
          if (hwndTemp)
            WinSetParent(hwndTemp, mFrameWnd, TRUE);
          hwndTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndSysMenu");
          if (hwndTemp)
            WinSetParent(hwndTemp, mFrameWnd, TRUE);
        }
        break;
      }
      case WM_ADJUSTFRAMEPOS:
      {
        if (mChromeHidden && ((PSWP)mp1)->fl & SWP_RESTORE) {
          HWND hwndTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndSysMenu");
          if (hwndTemp)
            WinSetParent(hwndTemp, HWND_OBJECT, TRUE);
          hwndTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndTitleBar");
          if (hwndTemp)
            WinSetParent(hwndTemp, HWND_OBJECT, TRUE);
          hwndTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndMinMax");
          if (hwndTemp)
            WinSetParent(hwndTemp, HWND_OBJECT, TRUE);
        }
        break;
      }

      case WM_DESTROY:
         DEBUGFOCUS(frame WM_DESTROY);
         WinSubclassWindow( mFrameWnd, fnwpDefFrame);
         WinSetWindowPtr( mFrameWnd, QWL_USER, 0);
         WinRemoveProperty(mFrameWnd, "hwndTitleBar");
         WinRemoveProperty(mFrameWnd, "hwndSysMenu");
         WinRemoveProperty(mFrameWnd, "hwndMinMax");
         WinRemoveProperty(mFrameWnd, "ulStyle");
         break;
      case WM_INITMENU:
         
         if (mChromeHidden) {
            if (WinQueryWindowULong(mFrameWnd, QWL_STYLE) & WS_MINIMIZED) {
              if (SHORT1FROMMP(mp1) == SC_SYSMENU) {
                MENUITEM menuitem;
                WinSendMsg(WinWindowFromID(mFrameWnd, FID_SYSMENU), MM_QUERYITEM, MPFROM2SHORT(SC_SYSMENU, FALSE), MPARAM(&menuitem));
                mresult = (*fnwpDefFrame)( mFrameWnd, msg, mp1, mp2);
                WinEnableMenuItem(menuitem.hwndSubMenu, SC_MAXIMIZE, FALSE);
                bDone = TRUE;
              }
            }
         }
         break;
      case WM_SYSCOMMAND:
         
         if (mChromeHidden) {
            if (WinQueryWindowULong(mFrameWnd, QWL_STYLE) & WS_MINIMIZED) {
              if ((SHORT1FROMMP(mp1) == SC_MAXIMIZE))
              {
                bDone = TRUE;
              }
            }
         }
         break;

      
      
      
      case WM_ACTIVATE:
         DEBUGFOCUS(WM_ACTIVATE);
         if (mp1) {
            mNeedActivation = PR_TRUE;
         } else {
            mNeedActivation = PR_FALSE;
            DEBUGFOCUS(NS_DEACTIVATE);
            DispatchFocus(NS_DEACTIVATE);
            
            
            
            WinSetWindowULong(mFrameWnd, QWL_HWNDFOCUSSAVE, 0);
         }
         break;
   }

   if( !bDone)
      mresult = (*fnwpDefFrame)( mFrameWnd, msg, mp1, mp2);

   return mresult;
}

