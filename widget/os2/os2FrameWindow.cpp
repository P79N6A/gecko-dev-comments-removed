














































#include "nsWindow.h"
#include "os2FrameWindow.h"
#include "nsIRollupListener.h"
#include "nsIScreenManager.h"
#include "nsOS2Uni.h"




extern nsIRollupListener*  gRollupListener;
extern nsIWidget*          gRollupWidget;
extern bool                gRollupConsumeRollupEvent;
extern PRUint32            gOS2Flags;

#ifdef DEBUG_FOCUS
  extern int currentWindowIdentifier;
#endif




#ifdef DEBUG_FOCUS
  #define DEBUGFOCUS(what) fprintf(stderr, "[%8x]  %8lx  (%02d)  "#what"\n", \
                                   (int)this, mFrameWnd, mWindowIdentifier)
#else
  #define DEBUGFOCUS(what)
#endif





os2FrameWindow::os2FrameWindow(nsWindow* aOwner)
{
  mOwner            = aOwner;
  mFrameWnd         = 0;
  mTitleBar         = 0;
  mSysMenu          = 0;
  mMinMax           = 0;
  mSavedStyle       = 0;
  mFrameIcon        = 0;
  mChromeHidden     = false;
  mNeedActivation   = false;
  mPrevFrameProc    = 0;
  mFrameBounds      = nsIntRect(0, 0, 0, 0);
}



os2FrameWindow::~os2FrameWindow()
{
  if (mFrameIcon) {
    WinFreeFileIcon(mFrameIcon);
    mFrameIcon = 0;
  }
}






HWND os2FrameWindow::CreateFrameWindow(nsWindow* aParent,
                                       HWND aParentWnd,
                                       const nsIntRect& aRect,
                                       nsWindowType aWindowType,
                                       nsBorderStyle aBorderStyle)
{
  
  HWND hClient;
  PRUint32 fcfFlags = GetFCFlags(aWindowType, aBorderStyle);
  mFrameWnd = WinCreateStdWindow(HWND_DESKTOP,
                                 0,
                                 (ULONG*)&fcfFlags,
                                 kWindowClassName,
                                 "Title",
                                 WS_CLIPCHILDREN,
                                 NULLHANDLE,
                                 0,
                                 &hClient);
  if (!mFrameWnd) {
    return 0;
  }

  
  SetWindowListVisibility(false);

  
  if (aParentWnd != HWND_DESKTOP) {
    WinSetOwner(mFrameWnd, aParentWnd);
  }

  
  mTitleBar = WinWindowFromID(mFrameWnd, FID_TITLEBAR);
  mSysMenu  = WinWindowFromID(mFrameWnd, FID_SYSMENU);
  mMinMax   = WinWindowFromID(mFrameWnd, FID_MINMAX);

  
  
  
  RECTL rcl = {0, 0, aRect.width, aRect.height};
  WinCalcFrameRect(mFrameWnd, &rcl, FALSE);
  mFrameBounds = nsIntRect(aRect.x, aRect.y,
                           rcl.xRight-rcl.xLeft, rcl.yTop-rcl.yBottom);

  
  PRInt32 pmY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN)
                - mFrameBounds.y - mFrameBounds.height;
  WinSetWindowPos(mFrameWnd, 0, mFrameBounds.x, pmY,
                  mFrameBounds.width, mFrameBounds.height,
                  SWP_SIZE | SWP_MOVE);

  
  
  SWP swp;
  WinQueryWindowPos(hClient, &swp);
  mOwner->SetBounds(nsIntRect(swp.x, mFrameBounds.height - swp.y - swp.cy,
                              swp.cx, swp.cy));

  
  mPrevFrameProc = WinSubclassWindow(mFrameWnd, fnwpFrame);
  WinSetWindowPtr(mFrameWnd, QWL_USER, this);

  DEBUGFOCUS(Create os2FrameWindow);
  return hClient;
}



PRUint32 os2FrameWindow::GetFCFlags(nsWindowType aWindowType,
                                    nsBorderStyle aBorderStyle)
{
  PRUint32 style = FCF_TITLEBAR | FCF_SYSMENU | FCF_TASKLIST |
                   FCF_CLOSEBUTTON | FCF_NOBYTEALIGN | FCF_AUTOICON;

  if (aWindowType == eWindowType_dialog) {
    if (aBorderStyle == eBorderStyle_default) {
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
  if (aWindowType == eWindowType_invisible) {
    style &= ~FCF_TASKLIST;
  }

  if (aBorderStyle != eBorderStyle_default &&
      aBorderStyle != eBorderStyle_all) {
    if (aBorderStyle == eBorderStyle_none ||
        !(aBorderStyle & eBorderStyle_resizeh)) {
      style &= ~FCF_SIZEBORDER;
      style |= FCF_DLGBORDER;
    }
    if (aBorderStyle == eBorderStyle_none ||
        !(aBorderStyle & eBorderStyle_border)) {
      style &= ~(FCF_DLGBORDER | FCF_SIZEBORDER);
    }
    if (aBorderStyle == eBorderStyle_none ||
        !(aBorderStyle & eBorderStyle_title)) {
      style &= ~(FCF_TITLEBAR | FCF_TASKLIST);
    }
    if (aBorderStyle == eBorderStyle_none ||
        !(aBorderStyle & eBorderStyle_close)) {
      style &= ~FCF_CLOSEBUTTON;
    }
    if (aBorderStyle == eBorderStyle_none ||
      !(aBorderStyle & (eBorderStyle_menu | eBorderStyle_close))) {
      style &= ~FCF_SYSMENU;
    }

    
    
    

    if (aBorderStyle == eBorderStyle_none ||
        !(aBorderStyle & eBorderStyle_minimize)) {
      style &= ~FCF_MINBUTTON;
    }
    if (aBorderStyle == eBorderStyle_none ||
        !(aBorderStyle & eBorderStyle_maximize)) {
      style &= ~FCF_MAXBUTTON;
    }
  }

  return style;
}







nsresult os2FrameWindow::Show(bool aState)
{
  PRUint32 ulFlags;
  if (!aState) {
    ulFlags = SWP_HIDE | SWP_DEACTIVATE;
  } else {
    ulFlags = SWP_SHOW | SWP_ACTIVATE;

    PRUint32 ulStyle = WinQueryWindowULong(mFrameWnd, QWL_STYLE);
    PRInt32 sizeMode;
    mOwner->GetSizeMode(&sizeMode);
    if (!(ulStyle & WS_VISIBLE)) {
      if (sizeMode == nsSizeMode_Maximized) {
        ulFlags |= SWP_MAXIMIZE;
      } else if (sizeMode == nsSizeMode_Minimized) {
        ulFlags |= SWP_MINIMIZE;
      } else {
        ulFlags |= SWP_RESTORE;
      }
    } else if (ulStyle & WS_MINIMIZED) {
      if (sizeMode == nsSizeMode_Maximized) {
        ulFlags |= SWP_MAXIMIZE;
      } else {
        ulFlags |= SWP_RESTORE;
      }
    }
  }

  WinSetWindowPos(mFrameWnd, 0, 0, 0, 0, 0, ulFlags);
  SetWindowListVisibility(aState);

  return NS_OK;
}



void os2FrameWindow::SetWindowListVisibility(bool aState)
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





nsresult os2FrameWindow::GetBounds(nsIntRect& aRect)
{
  aRect = mFrameBounds;
  return NS_OK;
}



nsresult os2FrameWindow::Move(PRInt32 aX, PRInt32 aY)
{
  aY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - mFrameBounds.height - aY;
  WinSetWindowPos(mFrameWnd, 0, aX, aY, 0, 0, SWP_MOVE);
  return NS_OK;
}



nsresult os2FrameWindow::Resize(PRInt32 aWidth, PRInt32 aHeight,
                                bool aRepaint)
{
  
  
  Resize(mFrameBounds.x, mFrameBounds.y, aWidth, aHeight, aRepaint);

  return NS_OK;
}



nsresult os2FrameWindow::Resize(PRInt32 aX, PRInt32 aY,
                                PRInt32 aWidth, PRInt32 aHeight,
                                bool aRepaint)
{
  aY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - aY - aHeight;
  WinSetWindowPos(mFrameWnd, 0, aX, aY, aWidth, aHeight, SWP_MOVE | SWP_SIZE);
  return NS_OK;
}










void os2FrameWindow::ActivateTopLevelWidget()
{
  
  
  
  if (mNeedActivation) {
    PRInt32 sizeMode;
    mOwner->GetSizeMode(&sizeMode);
    if (sizeMode != nsSizeMode_Minimized) {
      mNeedActivation = false;
      DEBUGFOCUS(NS_ACTIVATE);
      mOwner->DispatchActivationEvent(NS_ACTIVATE);
    }
  }
  return;
}







nsresult os2FrameWindow::SetSizeMode(PRInt32 aMode)
{
  PRInt32 previousMode;
  mOwner->GetSizeMode(&previousMode);

  
  nsresult rv = mOwner->nsBaseWidget::SetSizeMode(aMode);
  if (!NS_SUCCEEDED(rv)) {
    return rv;
  }

  
  
  
  if (previousMode == nsSizeMode_Minimized && previousMode != aMode) {
    DEBUGFOCUS(deferred NS_ACTIVATE);
    ActivateTopLevelWidget();
  }

  ULONG ulStyle = WinQueryWindowULong(mFrameWnd, QWL_STYLE);

  switch (aMode) {
    case nsSizeMode_Normal:
      if (ulStyle & (WS_MAXIMIZED | WS_MINIMIZED)) {
        WinSetWindowPos(mFrameWnd, 0, 0, 0, 0, 0, SWP_RESTORE);
      }
      break;

    case nsSizeMode_Minimized:
      if (!(ulStyle & WS_MINIMIZED)) {
        WinSetWindowPos(mFrameWnd, HWND_BOTTOM, 0, 0, 0, 0,
                        SWP_MINIMIZE | SWP_ZORDER | SWP_DEACTIVATE);
      }
      break;

    case nsSizeMode_Maximized:
      
      
      if (!(ulStyle & WS_MAXIMIZED) && !mChromeHidden) {
        WinSetWindowPos(mFrameWnd, HWND_TOP, 0, 0, 0, 0,
                        SWP_MAXIMIZE | SWP_ZORDER);
      }
      break;

    
    case nsSizeMode_Fullscreen:
    default:
      break;
  }

  return NS_OK;
}




nsresult os2FrameWindow::HideWindowChrome(bool aShouldHide)
{
  
  
  
  if (WinQueryWindowULong(mFrameWnd, QWL_STYLE) & WS_MAXIMIZED) {
    WinSetWindowPos(mFrameWnd, 0, 0, 0, 0, 0, SWP_RESTORE | SWP_NOREDRAW);
  }

  HWND hParent;
  if (aShouldHide) {
    hParent = HWND_OBJECT;
    mChromeHidden = true;
  } else {
    hParent = mFrameWnd;
    mChromeHidden = false;
  }

  
  WinSetParent(mTitleBar, hParent, FALSE);
  WinSetParent(mSysMenu, hParent, FALSE);
  WinSetParent(mMinMax, hParent, FALSE);

  
  if (aShouldHide) {
    mSavedStyle = WinQueryWindowULong(mFrameWnd, QWL_STYLE);
    WinSetWindowULong(mFrameWnd, QWL_STYLE, mSavedStyle & ~FS_SIZEBORDER);
    WinSendMsg(mFrameWnd, WM_UPDATEFRAME, 0, 0);
  } else {
    WinSetWindowULong(mFrameWnd, QWL_STYLE, mSavedStyle);
    WinSendMsg(mFrameWnd, WM_UPDATEFRAME,
               MPFROMLONG(FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX), 0);
  }

  return NS_OK;
}





#define MAX_TITLEBAR_LENGTH 256

nsresult os2FrameWindow::SetTitle(const nsAString& aTitle)
{
  PRUnichar* uchtemp = ToNewUnicode(aTitle);
  for (PRUint32 i = 0; i < aTitle.Length(); i++) {
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
  WinSetWindowText(mFrameWnd, title.Elements());

  
  if (mChromeHidden) {
    WinSetWindowText(mTitleBar, title.Elements());
  }

  nsMemory::Free(uchtemp);
  return NS_OK;
}





nsresult os2FrameWindow::SetIcon(const nsAString& aIconSpec)
{
  static HPOINTER hDefaultIcon = 0;
  HPOINTER        hWorkingIcon = 0;

  
  nsCOMPtr<nsILocalFile> iconFile;
  mOwner->ResolveIconName(aIconSpec, NS_LITERAL_STRING(".ico"),
                          getter_AddRefs(iconFile));

  
  if (iconFile) {
    nsCAutoString path;
    iconFile->GetNativePath(path);

    if (mFrameIcon) {
      WinFreeFileIcon(mFrameIcon);
    }
    mFrameIcon = WinLoadFileIcon(path.get(), FALSE);
    hWorkingIcon = mFrameIcon;
  }

  
  
  if (!hWorkingIcon) {
    if (!hDefaultIcon) {
      hDefaultIcon = WinLoadPointer(HWND_DESKTOP, 0, 1);
      if (!hDefaultIcon) {
        hDefaultIcon = WinQuerySysPointer(HWND_DESKTOP, SPTR_APPICON, FALSE);
      }
    }
    hWorkingIcon = hDefaultIcon;
  }

  WinSendMsg(mFrameWnd, WM_SETICON, (MPARAM)hWorkingIcon, (MPARAM)0);
  return NS_OK;
}




nsresult os2FrameWindow::ConstrainPosition(bool aAllowSlop,
                                      PRInt32* aX, PRInt32* aY)
{
  
  bool doConstrain = false;

  
  
  RECTL screenRect;

  nsCOMPtr<nsIScreenManager> screenmgr =
    do_GetService("@mozilla.org/gfx/screenmanager;1");
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    PRInt32 left, top, width, height;

    
    width = mFrameBounds.width > 0 ? mFrameBounds.width : 1;
    height = mFrameBounds.height > 0 ? mFrameBounds.height : 1;
    screenmgr->ScreenForRect(*aX, *aY, width, height,
                             getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailRect(&left, &top, &width, &height);
      screenRect.xLeft = left;
      screenRect.xRight = left+width;
      screenRect.yTop = top;
      screenRect.yBottom = top+height;
      doConstrain = true;
    }
  }

#define kWindowPositionSlop 100

  if (doConstrain) {
    if (aAllowSlop) {
      if (*aX < screenRect.xLeft - mFrameBounds.width + kWindowPositionSlop) {
        *aX = screenRect.xLeft - mFrameBounds.width + kWindowPositionSlop;
      } else
      if (*aX >= screenRect.xRight - kWindowPositionSlop) {
        *aX = screenRect.xRight - kWindowPositionSlop;
      }

      if (*aY < screenRect.yTop) {
        *aY = screenRect.yTop;
      } else
      if (*aY >= screenRect.yBottom - kWindowPositionSlop) {
        *aY = screenRect.yBottom - kWindowPositionSlop;
      }
    } else {
      if (*aX < screenRect.xLeft) {
        *aX = screenRect.xLeft;
      } else
      if (*aX >= screenRect.xRight - mFrameBounds.width) {
        *aX = screenRect.xRight - mFrameBounds.width;
      }

      if (*aY < screenRect.yTop) {
        *aY = screenRect.yTop;
      } else
      if (*aY >= screenRect.yBottom - mFrameBounds.height) {
        *aY = screenRect.yBottom - mFrameBounds.height;
      }
    }
  }

  return NS_OK;
}







MRESULT EXPENTRY fnwpFrame(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  
  if (gRollupListener && gRollupWidget) {
    if (msg == WM_TRACKFRAME || msg == WM_MINMAXFRAME ||
        msg == WM_BUTTON1DOWN || msg == WM_BUTTON2DOWN ||
        msg == WM_BUTTON3DOWN) {
      
      if (!nsWindow::EventIsInsideWindow((nsWindow*)gRollupWidget)) {
        gRollupListener->Rollup(PR_UINT32_MAX);
      }
    }
  }

  os2FrameWindow* pFrame = (os2FrameWindow*)WinQueryWindowPtr(hwnd, QWL_USER);
  return pFrame->ProcessFrameMessage(msg, mp1, mp2);
}




MRESULT os2FrameWindow::ProcessFrameMessage(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  MRESULT mresult = 0;
  bool    isDone = false;

  switch (msg) {
    case WM_WINDOWPOSCHANGED: {
      PSWP pSwp = (PSWP)mp1;

      
      
      if (pSwp->fl & SWP_MOVE && !(pSwp->fl & SWP_MINIMIZE)) {
        POINTL ptl = { pSwp->x, pSwp->y + pSwp->cy };
        ptl.y = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN) - ptl.y;
        mFrameBounds.x = ptl.x;
        mFrameBounds.y = ptl.y;
        mOwner->DispatchMoveEvent(ptl.x, ptl.y);
      }

      
      
      if (pSwp->fl & SWP_SIZE && !(pSwp->fl & SWP_MINIMIZE)) {
        mFrameBounds.width = pSwp->cx;
        mFrameBounds.height = pSwp->cy;
        mresult = (*mPrevFrameProc)(mFrameWnd, msg, mp1, mp2);
        isDone = true;
      }

      if (pSwp->fl & (SWP_MAXIMIZE | SWP_MINIMIZE | SWP_RESTORE)) {
        nsSizeModeEvent event(true, NS_SIZEMODE, mOwner);
        if (pSwp->fl & SWP_MAXIMIZE) {
          event.mSizeMode = nsSizeMode_Maximized;
        } else if (pSwp->fl & SWP_MINIMIZE) {
          event.mSizeMode = nsSizeMode_Minimized;
        } else {
          event.mSizeMode = nsSizeMode_Normal;
        }
        mOwner->InitEvent(event);
        mOwner->DispatchWindowEvent(&event);
      }
      break;
    }

     
     
     
     
     

    case WM_ADJUSTWINDOWPOS:
      if (mChromeHidden && ((PSWP)mp1)->fl & SWP_MINIMIZE) {
        WinSetParent(mTitleBar, mFrameWnd, TRUE);
        WinSetParent(mSysMenu, mFrameWnd, TRUE);
        WinSetParent(mMinMax, mFrameWnd, TRUE);
      }
      break;

    case WM_ADJUSTFRAMEPOS:
      if (mChromeHidden && ((PSWP)mp1)->fl & SWP_RESTORE) {
        WinSetParent(mTitleBar, HWND_OBJECT, TRUE);
        WinSetParent(mSysMenu, HWND_OBJECT, TRUE);
        WinSetParent(mMinMax, HWND_OBJECT, TRUE);
      }
      break;

    case WM_DESTROY:
      DEBUGFOCUS(frame WM_DESTROY);
      WinSubclassWindow(mFrameWnd, mPrevFrameProc);
      WinSetWindowPtr(mFrameWnd, QWL_USER, 0);
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
        isDone = true;
      }
      break;

    case WM_SYSCOMMAND:
      
      if (mChromeHidden &&
          SHORT1FROMMP(mp1) == SC_MAXIMIZE &&
          WinQueryWindowULong(mFrameWnd, QWL_STYLE) & WS_MINIMIZED) {
        isDone = true;
      }
      break;

    
    
    
    case WM_ACTIVATE:
      DEBUGFOCUS(WM_ACTIVATE);
      if (mp1) {
        mNeedActivation = true;
      } else {
        mNeedActivation = false;
        DEBUGFOCUS(NS_DEACTIVATE);
        mOwner->DispatchActivationEvent(NS_DEACTIVATE);
        
        
        
        WinSetWindowULong(mFrameWnd, QWL_HWNDFOCUSSAVE, 0);
      }
      break;
  }

  if (!isDone) {
    mresult = (*mPrevFrameProc)(mFrameWnd, msg, mp1, mp2);
  }

  return mresult;
}



