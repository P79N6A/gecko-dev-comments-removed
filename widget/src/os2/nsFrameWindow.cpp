














































#include "nsFrameWindow.h"
#include "nsIRollupListener.h"



extern nsIRollupListener*  gRollupListener;
extern nsIWidget*          gRollupWidget;
extern PRBool              gRollupConsumeRollupEvent;
extern PRUint32            gOS2Flags;

#ifdef DEBUG_FOCUS
  extern int currentWindowIdentifier;
#endif





nsFrameWindow::nsFrameWindow() : nsWindow()
{
  mPrevFrameProc = 0;
  mWindowType  = eWindowType_toplevel;
  mNeedActivation = PR_FALSE;
}

nsFrameWindow::~nsFrameWindow()
{
}




nsresult nsFrameWindow::CreateWindow(nsWindow* aParent,
                                     HWND aParentWnd,
                                     const nsIntRect& aRect,
                                     PRUint32 aStyle)
{
  
  
  mIsTopWidgetWindow = PR_TRUE;

  
  PRUint32 fcfFlags = GetFCFlags();
  mFrameWnd = WinCreateStdWindow(HWND_DESKTOP,
                                 0,
                                 (ULONG*)&fcfFlags,
                                 kWindowClassName,
                                 "Title",
                                 aStyle,
                                 NULLHANDLE,
                                 0,
                                 &mWnd);
  NS_ENSURE_TRUE(mFrameWnd, NS_ERROR_FAILURE);

  
  SetWindowListVisibility(PR_FALSE);

  
  if (aParentWnd != HWND_DESKTOP) {
    WinSetOwner(mFrameWnd, aParentWnd);
  }

  
  HWND hTemp = WinWindowFromID(mFrameWnd, FID_TITLEBAR);
  WinSetProperty(mFrameWnd, "hwndTitleBar", (PVOID)hTemp, 0);
  hTemp = WinWindowFromID(mFrameWnd, FID_SYSMENU);
  WinSetProperty(mFrameWnd, "hwndSysMenu", (PVOID)hTemp, 0);
  hTemp = WinWindowFromID(mFrameWnd, FID_MINMAX);
  WinSetProperty(mFrameWnd, "hwndMinMax", (PVOID)hTemp, 0);

  
  PRInt32 minHeight;
  if (fcfFlags & FCF_SIZEBORDER) {
    minHeight = 2 * WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER);
  } else if (fcfFlags & FCF_DLGBORDER) {
    minHeight = 2 * WinQuerySysValue(HWND_DESKTOP, SV_CYDLGFRAME);
  } else {
    minHeight = 2 * WinQuerySysValue(HWND_DESKTOP, SV_CYBORDER);
  }
  if (fcfFlags & FCF_TITLEBAR) {
    minHeight += WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);
  }

  
  
  mBounds = aRect;
  if (mBounds.height < minHeight) {
    mBounds.height = minHeight;
  }
  PRInt32 pmY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)
                - mBounds.y - mBounds.height;
  WinSetWindowPos(mFrameWnd, 0, mBounds.x, pmY,
                  mBounds.width, mBounds.height, SWP_SIZE | SWP_MOVE);
  UpdateClientSize();

  
  mPrevFrameProc = WinSubclassWindow(mFrameWnd, fnwpFrame);
  WinSetWindowPtr(mFrameWnd, QWL_USER, this);

  DEBUGFOCUS(Create nsFrameWindow);
  return NS_OK;
}



PRUint32 nsFrameWindow::GetFCFlags()
{
  PRUint32 style = FCF_TITLEBAR | FCF_SYSMENU | FCF_TASKLIST |
                   FCF_CLOSEBUTTON | FCF_NOBYTEALIGN | FCF_AUTOICON;

  if (mWindowType == eWindowType_dialog) {
    if (mBorderStyle == eBorderStyle_default) {
      style |= FCF_DLGBORDER;
    } else {
      style |= FCF_SIZEBORDER | FCF_MINMAX;
    }
  }
  else {
    style |= FCF_SIZEBORDER | FCF_MINMAX;
  }

  if (gOS2Flags & kIsDBCS) {
    style |= FCF_DBE_APPSTAT;
  }
  if (mWindowType == eWindowType_invisible) {
    style &= ~FCF_TASKLIST;
  }

  if (mBorderStyle != eBorderStyle_default &&
      mBorderStyle != eBorderStyle_all) {
    if (mBorderStyle == eBorderStyle_none ||
        !(mBorderStyle & eBorderStyle_resizeh)) {
      style &= ~FCF_SIZEBORDER;
      style |= FCF_DLGBORDER;
    }
    if (mBorderStyle == eBorderStyle_none ||
        !(mBorderStyle & eBorderStyle_border)) {
      style &= ~(FCF_DLGBORDER | FCF_SIZEBORDER);
    }
    if (mBorderStyle == eBorderStyle_none ||
        !(mBorderStyle & eBorderStyle_title)) {
      style &= ~(FCF_TITLEBAR | FCF_TASKLIST);
    }
    if (mBorderStyle == eBorderStyle_none ||
        !(mBorderStyle & eBorderStyle_close)) {
      style &= ~FCF_CLOSEBUTTON;
    }
    if (mBorderStyle == eBorderStyle_none ||
      !(mBorderStyle & (eBorderStyle_menu | eBorderStyle_close))) {
      style &= ~FCF_SYSMENU;
    }

    
    
    

    if (mBorderStyle == eBorderStyle_none ||
        !(mBorderStyle & eBorderStyle_minimize)) {
      style &= ~FCF_MINBUTTON;
    }
    if (mBorderStyle == eBorderStyle_none ||
        !(mBorderStyle & eBorderStyle_maximize)) {
      style &= ~FCF_MAXBUTTON;
    }
  }

  return style;
}







NS_METHOD nsFrameWindow::Show(PRBool aState)
{
  if (!mWnd) {
    return NS_OK;
  }

  PRUint32 ulFlags;
  if (!aState) {
    ulFlags = SWP_HIDE | SWP_DEACTIVATE;
  } else {
    ulFlags = SWP_SHOW;

    
    if (WinIsWindowVisible(WinQueryWindow(mFrameWnd, QW_PARENT))) {
      ulFlags |= SWP_ACTIVATE;
    }

    PRUint32 ulStyle = WinQueryWindowULong(mFrameWnd, QWL_STYLE);
    if (!(ulStyle & WS_VISIBLE)) {
      PRInt32 sizeMode;
      GetSizeMode(&sizeMode);
      if (sizeMode == nsSizeMode_Maximized) {
        ulFlags |= SWP_MAXIMIZE;
      } else if (sizeMode == nsSizeMode_Minimized) {
        ulFlags |= SWP_MINIMIZE;
      } else {
        ulFlags |= SWP_RESTORE;
      }
    }
    if (ulStyle & WS_MINIMIZED) {
      ulFlags |= (SWP_RESTORE | SWP_MAXIMIZE);
    }
  }

  WinSetWindowPos(mFrameWnd, 0, 0, 0, 0, 0, ulFlags);
  SetWindowListVisibility(aState);

  return NS_OK;
}



NS_METHOD nsFrameWindow::GetClientBounds(nsIntRect& aRect)
{
  RECTL rcl = { 0, 0, mBounds.width, mBounds.height };
  WinCalcFrameRect(mFrameWnd, &rcl, TRUE); 
  aRect.x = rcl.xLeft;
  aRect.y = mBounds.height - rcl.yTop;
  aRect.width = mSizeClient.width;
  aRect.height = mSizeClient.height;
  return NS_OK;
}



void nsFrameWindow::UpdateClientSize()
{
  RECTL rcl = { 0, 0, mBounds.width, mBounds.height };
  WinCalcFrameRect(mFrameWnd, &rcl, TRUE); 
  mSizeClient.width  = rcl.xRight - rcl.xLeft;
  mSizeClient.height = rcl.yTop - rcl.yBottom;
}







void nsFrameWindow::ActivateTopLevelWidget()
{
  
  
  

  if (mNeedActivation) {
    PRInt32 sizeMode;
    GetSizeMode(&sizeMode);
    if (sizeMode != nsSizeMode_Minimized) {
      mNeedActivation = PR_FALSE;
      DEBUGFOCUS(NS_ACTIVATE);
      DispatchActivationEvent(NS_ACTIVATE);
    }
  }
  return;
}




PRBool nsFrameWindow::OnReposition(PSWP pSwp)
{
  return PR_TRUE;
}



void nsFrameWindow::SetWindowListVisibility(PRBool aState)
{
  HSWITCH hswitch = WinQuerySwitchHandle(mFrameWnd, 0);
  if (hswitch) {
    SWCNTRL swctl;
    WinQuerySwitchEntry(hswitch, &swctl);
    swctl.uchVisibility = aState ? SWL_VISIBLE : SWL_INVISIBLE;
    swctl.fbJump        = aState ? SWL_JUMPABLE : SWL_NOTJUMPABLE;
    WinChangeSwitchEntry(hswitch, &swctl);
  }
}







MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  
  if (gRollupListener && gRollupWidget) {
    if (msg == WM_TRACKFRAME || msg == WM_MINMAXFRAME ||
        msg == WM_BUTTON1DOWN || msg == WM_BUTTON2DOWN ||
        msg == WM_BUTTON3DOWN) {
      
      if (!nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget)) {
        gRollupListener->Rollup(PR_UINT32_MAX, nsnull);
      }
    }
  }

  nsFrameWindow* pFrame = (nsFrameWindow*)WinQueryWindowPtr(hwnd, QWL_USER);
  return pFrame->FrameMessage(msg, mp1, mp2);
}




MRESULT nsFrameWindow::FrameMessage(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  MRESULT mresult = 0;
  PRBool  bDone = PR_FALSE;

  switch (msg) {
    case WM_WINDOWPOSCHANGED: {
      PSWP pSwp = (PSWP)mp1;

      
      if (pSwp->fl & SWP_MOVE && !(pSwp->fl & SWP_MINIMIZE)) {
        POINTL ptl = { pSwp->x, pSwp->y + pSwp->cy };
        ptl.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - ptl.y;
        mBounds.x = ptl.x;
        mBounds.y = ptl.y;
        DispatchMoveEvent(ptl.x, ptl.y);
      }

      
      if (pSwp->fl & SWP_SIZE && !(pSwp->fl & SWP_MINIMIZE)) {
        mresult = (*mPrevFrameProc)(mFrameWnd, msg, mp1, mp2);
        bDone = PR_TRUE;
        mBounds.width = pSwp->cx;
        mBounds.height = pSwp->cy;
        UpdateClientSize();
        DispatchResizeEvent(mSizeClient.width, mSizeClient.height);
      }

      if (pSwp->fl & (SWP_MAXIMIZE | SWP_MINIMIZE | SWP_RESTORE)) {
        nsSizeModeEvent event(PR_TRUE, NS_SIZEMODE, this);
        if (pSwp->fl & SWP_MAXIMIZE) {
          event.mSizeMode = nsSizeMode_Maximized;
        } else if (pSwp->fl & SWP_MINIMIZE) {
          event.mSizeMode = nsSizeMode_Minimized;
        } else {
          event.mSizeMode = nsSizeMode_Normal;
        }
        InitEvent(event);
        DispatchWindowEvent(&event);
      }

      break;
    }

     
     
     
     
     

    case WM_ADJUSTWINDOWPOS: {
      if (mChromeHidden && ((PSWP)mp1)->fl & SWP_MINIMIZE) {
        HWND hTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndMinMax");
        if (hTemp) {
          WinSetParent(hTemp, mFrameWnd, TRUE);
        }
        hTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndTitleBar");
        if (hTemp) {
          WinSetParent(hTemp, mFrameWnd, TRUE);
        }
        hTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndSysMenu");
        if (hTemp) {
          WinSetParent(hTemp, mFrameWnd, TRUE);
        }
      }
      break;
    }

    case WM_ADJUSTFRAMEPOS: {
      if (mChromeHidden && ((PSWP)mp1)->fl & SWP_RESTORE) {
        HWND hTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndSysMenu");
        if (hTemp) {
          WinSetParent(hTemp, HWND_OBJECT, TRUE);
        }
        hTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndTitleBar");
        if (hTemp) {
          WinSetParent(hTemp, HWND_OBJECT, TRUE);
        }
        hTemp = (HWND)WinQueryProperty(mFrameWnd, "hwndMinMax");
        if (hTemp) {
          WinSetParent(hTemp, HWND_OBJECT, TRUE);
        }
      }
      break;
    }

    case WM_DESTROY:
      DEBUGFOCUS(frame WM_DESTROY);
      WinSubclassWindow(mFrameWnd, mPrevFrameProc);
      WinSetWindowPtr(mFrameWnd, QWL_USER, 0);
      WinRemoveProperty(mFrameWnd, "hwndTitleBar");
      WinRemoveProperty(mFrameWnd, "hwndSysMenu");
      WinRemoveProperty(mFrameWnd, "hwndMinMax");
      WinRemoveProperty(mFrameWnd, "ulStyle");
      break;

    case WM_INITMENU:
      
      if (mChromeHidden &&
          SHORT1FROMMP(mp1) == SC_SYSMENU &&
          WinQueryWindowULong(mFrameWnd, QWL_STYLE) & WS_MINIMIZED) {
        MENUITEM menuitem;
        WinSendMsg(WinWindowFromID(mFrameWnd, FID_SYSMENU), MM_QUERYITEM,
                   MPFROM2SHORT(SC_SYSMENU, FALSE), MPARAM(&menuitem));
        mresult = (*mPrevFrameProc)(mFrameWnd, msg, mp1, mp2);
        WinEnableMenuItem(menuitem.hwndSubMenu, SC_MAXIMIZE, FALSE);
        bDone = PR_TRUE;
      }
      break;

    case WM_SYSCOMMAND:
      
      if (mChromeHidden &&
          SHORT1FROMMP(mp1) == SC_MAXIMIZE &&
          WinQueryWindowULong(mFrameWnd, QWL_STYLE) & WS_MINIMIZED) {
        bDone = PR_TRUE;
      }
      break;

    
    
    
    case WM_ACTIVATE:
      DEBUGFOCUS(WM_ACTIVATE);
      if (mp1) {
        mNeedActivation = PR_TRUE;
      } else {
        mNeedActivation = PR_FALSE;
        DEBUGFOCUS(NS_DEACTIVATE);
        DispatchActivationEvent(NS_DEACTIVATE);
        
        
        
        WinSetWindowULong(mFrameWnd, QWL_HWNDFOCUSSAVE, 0);
      }
      break;
  }

  if (!bDone) {
    mresult = (*mPrevFrameProc)(mFrameWnd, msg, mp1, mp2);
  }

  return mresult;
}



