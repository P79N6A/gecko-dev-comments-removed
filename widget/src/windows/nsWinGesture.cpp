









































#include "nscore.h"
#include "nsWinGesture.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIServiceManager.h"
#include "nsIDOMSimpleGestureEvent.h"
#include "nsGUIEvent.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const PRUnichar nsWinGesture::kGestureLibraryName[] =  L"user32.dll";
const PRUnichar nsWinGesture::kThemeLibraryName[] =  L"uxtheme.dll";
HMODULE nsWinGesture::sLibraryHandle = nsnull;
nsWinGesture::GetGestureInfoPtr nsWinGesture::getGestureInfo = nsnull;
nsWinGesture::CloseGestureInfoHandlePtr nsWinGesture::closeGestureInfoHandle = nsnull;
nsWinGesture::GetGestureExtraArgsPtr nsWinGesture::getGestureExtraArgs = nsnull;
nsWinGesture::SetGestureConfigPtr nsWinGesture::setGestureConfig = nsnull;
nsWinGesture::GetGestureConfigPtr nsWinGesture::getGestureConfig = nsnull;
nsWinGesture::BeginPanningFeedbackPtr nsWinGesture::beginPanningFeedback = nsnull;
nsWinGesture::EndPanningFeedbackPtr nsWinGesture::endPanningFeedback = nsnull;
nsWinGesture::UpdatePanningFeedbackPtr nsWinGesture::updatePanningFeedback = nsnull;
static PRBool gEnableSingleFingerPanEvents = PR_FALSE;

nsWinGesture::nsWinGesture() :
  mFeedbackActive(PR_FALSE),
  mXAxisFeedback(PR_FALSE),
  mYAxisFeedback(PR_FALSE),
  mPanActive(PR_FALSE),
  mPanInertiaActive(PR_FALSE)
{
  (void)InitLibrary();
  mPixelScrollOverflow = 0;
}



PRBool nsWinGesture::InitLibrary()
{
#ifdef WINCE
  return PR_FALSE;
#else
  if (getGestureInfo) {
    return PR_TRUE;
  } else if (sLibraryHandle) {
    return PR_FALSE;
  }

  sLibraryHandle = ::LoadLibraryW(kGestureLibraryName);
  HMODULE hTheme = ::LoadLibraryW(kThemeLibraryName);

  
  if (sLibraryHandle) {
    getGestureInfo = (GetGestureInfoPtr)GetProcAddress(sLibraryHandle, "GetGestureInfo");
    closeGestureInfoHandle = (CloseGestureInfoHandlePtr)GetProcAddress(sLibraryHandle, "CloseGestureInfoHandle");
    getGestureExtraArgs = (GetGestureExtraArgsPtr)GetProcAddress(sLibraryHandle, "GetGestureExtraArgs");
    setGestureConfig = (SetGestureConfigPtr)GetProcAddress(sLibraryHandle, "SetGestureConfig");
    getGestureConfig = (GetGestureConfigPtr)GetProcAddress(sLibraryHandle, "GetGestureConfig");
  }

  if (!getGestureInfo || !closeGestureInfoHandle || !getGestureExtraArgs ||
    !setGestureConfig || !getGestureConfig) {
    getGestureInfo         = nsnull;
    closeGestureInfoHandle = nsnull;
    getGestureExtraArgs    = nsnull;
    setGestureConfig       = nsnull;
    getGestureConfig       = nsnull;
    return PR_FALSE;
  }

  
  if (hTheme) {
    beginPanningFeedback = (BeginPanningFeedbackPtr)GetProcAddress(hTheme, "BeginPanningFeedback");
    endPanningFeedback = (EndPanningFeedbackPtr)GetProcAddress(hTheme, "EndPanningFeedback");
    updatePanningFeedback = (UpdatePanningFeedbackPtr)GetProcAddress(hTheme, "UpdatePanningFeedback");
  }

  if (!beginPanningFeedback || !endPanningFeedback || !updatePanningFeedback) {
    beginPanningFeedback   = nsnull;
    endPanningFeedback     = nsnull;
    updatePanningFeedback  = nsnull;
  }

  
  
  nsCOMPtr<nsIPrefService> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    nsCOMPtr<nsIPrefBranch> prefBranch;
    prefs->GetBranch(0, getter_AddRefs(prefBranch));
    if (prefBranch) {
      PRBool flag;
      if (NS_SUCCEEDED(prefBranch->GetBoolPref("gestures.enable_single_finger_input", &flag))
          && flag)
        gEnableSingleFingerPanEvents = PR_TRUE;
    }
  }

  return PR_TRUE;
#endif
}

#define GCOUNT 5

PRBool nsWinGesture::SetWinGestureSupport(HWND hWnd, nsGestureNotifyEvent::ePanDirection aDirection)
{
  if (!getGestureInfo)
    return PR_FALSE;

  GESTURECONFIG config[GCOUNT];

  memset(&config, 0, sizeof(config));

  config[0].dwID = GID_ZOOM;
  config[0].dwWant = GC_ZOOM;
  config[0].dwBlock = 0;

  config[1].dwID = GID_ROTATE;
  config[1].dwWant = GC_ROTATE;
  config[1].dwBlock = 0;

  config[2].dwID = GID_PAN;
  config[2].dwWant  = GC_PAN|GC_PAN_WITH_INERTIA|
                      GC_PAN_WITH_GUTTER;
  config[2].dwBlock = GC_PAN_WITH_SINGLE_FINGER_VERTICALLY|
                      GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;

  if (gEnableSingleFingerPanEvents) {

    if (aDirection == nsGestureNotifyEvent::ePanVertical ||
        aDirection == nsGestureNotifyEvent::ePanBoth)
    {
      config[2].dwWant  |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
      config[2].dwBlock -= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
    }

    if (aDirection == nsGestureNotifyEvent::ePanHorizontal ||
        aDirection == nsGestureNotifyEvent::ePanBoth)
    {
      config[2].dwWant  |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
      config[2].dwBlock -= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
    }

  }

  config[3].dwWant = GC_TWOFINGERTAP;
  config[3].dwID = GID_TWOFINGERTAP;
  config[3].dwBlock = 0;

  config[4].dwWant = GC_PRESSANDTAP;
  config[4].dwID = GID_PRESSANDTAP;
  config[4].dwBlock = 0;

  return SetGestureConfig(hWnd, GCOUNT, (PGESTURECONFIG)&config);
}



PRBool nsWinGesture::IsAvailable()
{
  return getGestureInfo != nsnull;
}

PRBool nsWinGesture::GetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo)
{
  if (!getGestureInfo || !hGestureInfo || !pGestureInfo)
    return PR_FALSE;

  ZeroMemory(pGestureInfo, sizeof(GESTUREINFO));
  pGestureInfo->cbSize = sizeof(GESTUREINFO);

  return getGestureInfo(hGestureInfo, pGestureInfo);
}

PRBool nsWinGesture::CloseGestureInfoHandle(HGESTUREINFO hGestureInfo)
{
  if (!getGestureInfo || !hGestureInfo)
    return PR_FALSE;

  return closeGestureInfoHandle(hGestureInfo);
}

PRBool nsWinGesture::GetGestureExtraArgs(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs)
{
  if (!getGestureInfo || !hGestureInfo || !pExtraArgs)
    return PR_FALSE;

  return getGestureExtraArgs(hGestureInfo, cbExtraArgs, pExtraArgs);
}

PRBool nsWinGesture::SetGestureConfig(HWND hWnd, UINT cIDs, PGESTURECONFIG pGestureConfig)
{
  if (!getGestureInfo || !pGestureConfig)
    return PR_FALSE;

  return setGestureConfig(hWnd, 0, cIDs, pGestureConfig, sizeof(GESTURECONFIG));
}

PRBool nsWinGesture::GetGestureConfig(HWND hWnd, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig)
{
  if (!getGestureInfo || !pGestureConfig)
    return PR_FALSE;

  return getGestureConfig(hWnd, 0, dwFlags, pcIDs, pGestureConfig, sizeof(GESTURECONFIG));
}

PRBool nsWinGesture::BeginPanningFeedback(HWND hWnd)
{
  if (!beginPanningFeedback)
    return PR_FALSE;

  return beginPanningFeedback(hWnd);
}

PRBool nsWinGesture::EndPanningFeedback(HWND hWnd)
{
  if (!beginPanningFeedback)
    return PR_FALSE;

  return endPanningFeedback(hWnd, TRUE);
}

PRBool nsWinGesture::UpdatePanningFeedback(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia)
{
  if (!beginPanningFeedback)
    return PR_FALSE;

  return updatePanningFeedback(hWnd, offsetX, offsetY, fInInertia);
}

PRBool nsWinGesture::IsPanEvent(LPARAM lParam)
{
  GESTUREINFO gi;

  ZeroMemory(&gi,sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);

  BOOL result = GetGestureInfo((HGESTUREINFO)lParam, &gi);
  if (!result)
    return PR_FALSE;

  if (gi.dwID == GID_PAN)
    return PR_TRUE;

  return PR_FALSE;
}



PRBool
nsWinGesture::ProcessGestureMessage(HWND hWnd, WPARAM wParam, LPARAM lParam, nsSimpleGestureEvent& evt)
{
  GESTUREINFO gi;

  ZeroMemory(&gi,sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);

  BOOL result = GetGestureInfo((HGESTUREINFO)lParam, &gi);
  if (!result)
    return PR_FALSE;

  
  nsPointWin coord;
  coord = gi.ptsLocation;
  coord.ScreenToClient(hWnd);

  evt.refPoint.x = coord.x;
  evt.refPoint.y = coord.y;

  
  
  switch(gi.dwID)
  {
    case GID_BEGIN:
    case GID_END:
      
      return PR_FALSE;
      break;

    case GID_ZOOM:
    {
      if (gi.dwFlags & GF_BEGIN) {
        

        
        mZoomIntermediate = (float)gi.ullArguments;

        evt.message = NS_SIMPLE_GESTURE_MAGNIFY_START;
        evt.delta = 0.0;
      }
      else {
        
        
        evt.message = NS_SIMPLE_GESTURE_MAGNIFY_UPDATE;
        
        evt.delta = -1.0 * (mZoomIntermediate - (float)gi.ullArguments);
        mZoomIntermediate = (float)gi.ullArguments;
      }
    }
    break;

    case GID_ROTATE:
    {
      
      double radians = 0.0;

      
      
      
      if (gi.ullArguments != 0)
        radians = GID_ROTATE_ANGLE_FROM_ARGUMENT(gi.ullArguments);

      double degrees = -1 * radians * (180/M_PI);

      if (gi.dwFlags & GF_BEGIN) {
          
          
          degrees = mRotateIntermediate = 0.0;
      }

      evt.direction = 0;
      evt.delta = degrees - mRotateIntermediate;
      mRotateIntermediate = degrees;

      if (evt.delta > 0)
        evt.direction = nsIDOMSimpleGestureEvent::ROTATION_COUNTERCLOCKWISE;
      else if (evt.delta < 0)
        evt.direction = nsIDOMSimpleGestureEvent::ROTATION_CLOCKWISE;

      if (gi.dwFlags & GF_BEGIN)
        evt.message = NS_SIMPLE_GESTURE_ROTATE_START;
      else if (gi.dwFlags & GF_END)
        evt.message = NS_SIMPLE_GESTURE_ROTATE;
      else
        evt.message = NS_SIMPLE_GESTURE_ROTATE_UPDATE;
    }
    break;

    case GID_TWOFINGERTAP:
    {
      
      
      evt.message = NS_SIMPLE_GESTURE_TAP;
    }
    break;

    case GID_PRESSANDTAP:
    {
      
      evt.message = NS_SIMPLE_GESTURE_PRESSTAP;
    }
    break;
  }

  return PR_TRUE;
}

PRBool
nsWinGesture::ProcessPanMessage(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  GESTUREINFO gi;

  ZeroMemory(&gi,sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);

  BOOL result = GetGestureInfo((HGESTUREINFO)lParam, &gi);
  if (!result)
    return PR_FALSE;

  
  nsPointWin coord;
  coord = mPanRefPoint = gi.ptsLocation;
  
  
  mPanRefPoint.ScreenToClient(hWnd);

  switch(gi.dwID)
  {
    case GID_BEGIN:
    case GID_END:
      
      return PR_FALSE;
      break;

    
    case GID_PAN:
    {
      if (gi.dwFlags & GF_BEGIN) {
        mPanIntermediate = coord;
        mPixelScrollDelta = 0;
        mPanActive = PR_TRUE;
        mPanInertiaActive = PR_FALSE;
      }
      else {

#ifdef DBG_jimm
        PRInt32 deltaX = mPanIntermediate.x - coord.x;
        PRInt32 deltaY = mPanIntermediate.y - coord.y;
        printf("coordX=%d coordY=%d deltaX=%d deltaY=%d x:%d y:%d\n", coord.x,
          coord.y, deltaX, deltaY, mXAxisFeedback, mYAxisFeedback);
#endif

        mPixelScrollDelta.x = mPanIntermediate.x - coord.x;
        mPixelScrollDelta.y = mPanIntermediate.y - coord.y;
        mPanIntermediate = coord;

        if (gi.dwFlags & GF_INERTIA)
          mPanInertiaActive = PR_TRUE;

        if (gi.dwFlags & GF_END) {
          mPanActive = PR_FALSE;
          mPanInertiaActive = PR_FALSE;
          PanFeedbackFinalize(hWnd, PR_TRUE);
        }
      }
    }
    break;
  }
  return PR_TRUE;
}

inline PRBool TestTransition(PRInt32 a, PRInt32 b)
{
  
  
  if (a == 0 || b == 0) return PR_TRUE;
  
  return (a < 0) == (b < 0);
}

void
nsWinGesture::UpdatePanFeedbackX(HWND hWnd, PRInt32 scrollOverflow, PRBool& endFeedback)
{
  
  
  if (scrollOverflow != 0) {
    if (!mFeedbackActive) {
      BeginPanningFeedback(hWnd);
      mFeedbackActive = PR_TRUE;
    }      
    endFeedback = PR_FALSE;
    mXAxisFeedback = PR_TRUE;
    return;
  }
  
  if (mXAxisFeedback) {
    PRInt32 newOverflow = mPixelScrollOverflow.x - mPixelScrollDelta.x;

    
    
    if (!TestTransition(newOverflow, mPixelScrollOverflow.x) || newOverflow == 0)
      return;

    
    mPixelScrollOverflow.x = newOverflow;
    endFeedback = PR_FALSE;
  }
}

void
nsWinGesture::UpdatePanFeedbackY(HWND hWnd, PRInt32 scrollOverflow, PRBool& endFeedback)
{
  
  
  if (scrollOverflow != 0) {
    if (!mFeedbackActive) {
      BeginPanningFeedback(hWnd);
      mFeedbackActive = PR_TRUE;
    }
    endFeedback = PR_FALSE;
    mYAxisFeedback = PR_TRUE;
    return;
  }
  
  if (mYAxisFeedback) {
    PRInt32 newOverflow = mPixelScrollOverflow.y - mPixelScrollDelta.y;

    
    
    if (!TestTransition(newOverflow, mPixelScrollOverflow.y) || newOverflow == 0)
      return;

    
    mPixelScrollOverflow.y = newOverflow;
    endFeedback = PR_FALSE;
  }
}

void
nsWinGesture::PanFeedbackFinalize(HWND hWnd, PRBool endFeedback)
{
  if (!mFeedbackActive)
    return;

  if (endFeedback) {
    mFeedbackActive = PR_FALSE;
    mXAxisFeedback = PR_FALSE;
    mYAxisFeedback = PR_FALSE;
    mPixelScrollOverflow = 0;
    EndPanningFeedback(hWnd);
    return;
  }

  UpdatePanningFeedback(hWnd, mPixelScrollOverflow.x, mPixelScrollOverflow.y, mPanInertiaActive);
}

PRBool
nsWinGesture::PanDeltaToPixelScrollX(nsMouseScrollEvent& evt)
{
  evt.delta = 0;
  evt.scrollOverflow = 0;

  
  
  
  if (mXAxisFeedback)
    return PR_FALSE;

  if (mPixelScrollDelta.x != 0)
  {
    evt.scrollFlags = nsMouseScrollEvent::kIsHorizontal|
                      nsMouseScrollEvent::kHasPixels|
                      nsMouseScrollEvent::kNoLines|
                      nsMouseScrollEvent::kNoDefer;
    evt.delta = mPixelScrollDelta.x;
    evt.refPoint.x = mPanRefPoint.x;
    evt.refPoint.y = mPanRefPoint.y;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsWinGesture::PanDeltaToPixelScrollY(nsMouseScrollEvent& evt)
{
  evt.delta = 0;
  evt.scrollOverflow = 0;

  
  
  
  if (mYAxisFeedback)
    return PR_FALSE;

  if (mPixelScrollDelta.y != 0)
  {
    evt.scrollFlags = nsMouseScrollEvent::kIsVertical|
                      nsMouseScrollEvent::kHasPixels|
                      nsMouseScrollEvent::kNoLines|
                      nsMouseScrollEvent::kNoDefer;
    evt.delta = mPixelScrollDelta.y;
    evt.refPoint.x = mPanRefPoint.x;
    evt.refPoint.y = mPanRefPoint.y;
    return PR_TRUE;
  }
  return PR_FALSE;
}
