





































#ifndef WinGesture_cpp__
#define WinGesture_cpp__

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
HMODULE nsWinGesture::sLibraryHandle = nsnull;
nsWinGesture::GetGestureInfoPtr nsWinGesture::getGestureInfo = nsnull;
nsWinGesture::CloseGestureInfoHandlePtr nsWinGesture::closeGestureInfoHandle = nsnull;
nsWinGesture::GetGestureExtraArgsPtr nsWinGesture::getGestureExtraArgs = nsnull;
nsWinGesture::SetGestureConfigPtr nsWinGesture::setGestureConfig = nsnull;
nsWinGesture::GetGestureConfigPtr nsWinGesture::getGestureConfig = nsnull;
static PRBool gEnableSingleFingerPanEvents = PR_FALSE;

nsWinGesture::nsWinGesture() :
  mAvailable(PR_FALSE)
{
  (void)InitLibrary();
}

nsWinGesture::~nsWinGesture()
{
  ShutdownLibrary();
}



PRBool nsWinGesture::InitLibrary()
{
#ifdef WINCE
  return PR_FALSE;
#else
  if (getGestureInfo) {
    mAvailable = PR_TRUE;
    return PR_TRUE;
  } else if (sLibraryHandle) {
    return PR_FALSE;
  }

  sLibraryHandle = ::LoadLibraryW(kGestureLibraryName);
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

  mAvailable = PR_TRUE;

  
  
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

void nsWinGesture::ShutdownLibrary()
{
  mAvailable = PR_FALSE;
}

#define GCOUNT 5

PRBool nsWinGesture::InitWinGestureSupport(HWND hWnd)
{
  if (!mAvailable)
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
  if (gEnableSingleFingerPanEvents) {
    config[2].dwWant = GC_PAN|GC_PAN_WITH_INERTIA|
                       GC_PAN_WITH_GUTTER|
                       GC_PAN_WITH_SINGLE_FINGER_VERTICALLY|
                       GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
    config[2].dwBlock = 0;
  }
  else {
    config[2].dwWant = GC_PAN|GC_PAN_WITH_INERTIA|
                       GC_PAN_WITH_GUTTER;
    config[2].dwBlock = GC_PAN_WITH_SINGLE_FINGER_VERTICALLY|
                        GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
  }

  config[3].dwWant = GC_TWOFINGERTAP;
  config[3].dwID = GID_TWOFINGERTAP;
  config[3].dwBlock = 0;

  config[4].dwWant = GC_ROLLOVER;
  config[4].dwID = GID_ROLLOVER;
  config[4].dwBlock = 0;

  return SetGestureConfig(hWnd, GCOUNT, (PGESTURECONFIG)&config);
}



PRBool nsWinGesture::IsAvailable()
{
  return mAvailable;
}

PRBool nsWinGesture::GetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo)
{
  if (!mAvailable || !hGestureInfo || !pGestureInfo)
    return PR_FALSE;

  ZeroMemory(pGestureInfo, sizeof(GESTUREINFO));
  pGestureInfo->cbSize = sizeof(GESTUREINFO);

  return getGestureInfo(hGestureInfo, pGestureInfo);
}

PRBool nsWinGesture::CloseGestureInfoHandle(HGESTUREINFO hGestureInfo)
{
  if (!mAvailable || !hGestureInfo)
    return PR_FALSE;

  return closeGestureInfoHandle(hGestureInfo);
}

PRBool nsWinGesture::GetGestureExtraArgs(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs)
{
  if (!mAvailable || !hGestureInfo || !pExtraArgs)
    return PR_FALSE;

  return getGestureExtraArgs(hGestureInfo, cbExtraArgs, pExtraArgs);
}

PRBool nsWinGesture::SetGestureConfig(HWND hWnd, UINT cIDs, PGESTURECONFIG pGestureConfig)
{
  if (!mAvailable || !pGestureConfig)
    return PR_FALSE;

  return setGestureConfig(hWnd, 0, cIDs, pGestureConfig, sizeof(GESTURECONFIG));
}

PRBool nsWinGesture::GetGestureConfig(HWND hWnd, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig)
{
  if (!mAvailable || !pGestureConfig)
    return PR_FALSE;

  return getGestureConfig(hWnd, 0, dwFlags, pcIDs, pGestureConfig, sizeof(GESTURECONFIG));
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

    case GID_ROLLOVER:
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
  coord = gi.ptsLocation;
  coord.ScreenToClient(hWnd);

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
      } else {
        mPixelScrollDelta.x = mPanIntermediate.x - coord.x;
        mPixelScrollDelta.y = mPanIntermediate.y - coord.y;
        mPanIntermediate = coord;
      }
    }
    break;
  }
  return PR_TRUE;
}

PRBool
nsWinGesture::PanDeltaToPixelScrollX(nsMouseScrollEvent& evt)
{
  if (mPixelScrollDelta.x != 0)
  {
    evt.scrollFlags = nsMouseScrollEvent::kIsHorizontal|nsMouseScrollEvent::kHasPixels|nsMouseScrollEvent::kNoLines;
    evt.delta = mPixelScrollDelta.x;
    evt.refPoint.x = mPanIntermediate.x;
    evt.refPoint.y = mPanIntermediate.y;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsWinGesture::PanDeltaToPixelScrollY(nsMouseScrollEvent& evt)
{
  if (mPixelScrollDelta.y != 0)
  {
    evt.scrollFlags = nsMouseScrollEvent::kIsVertical|nsMouseScrollEvent::kHasPixels|nsMouseScrollEvent::kNoLines;
    evt.delta = mPixelScrollDelta.y;
    evt.refPoint.x = mPanIntermediate.x;
    evt.refPoint.y = mPanIntermediate.y;
    return PR_TRUE;
  }
  return PR_FALSE;
}

#endif 


