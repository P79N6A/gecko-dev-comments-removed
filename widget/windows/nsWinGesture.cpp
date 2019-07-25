









































#include "nscore.h"
#include "nsWinGesture.h"
#include "nsUXThemeData.h"
#include "nsIDOMSimpleGestureEvent.h"
#include "nsGUIEvent.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* gWindowsLog;
#endif

const PRUnichar nsWinGesture::kGestureLibraryName[] =  L"user32.dll";
HMODULE nsWinGesture::sLibraryHandle = nsnull;
nsWinGesture::GetGestureInfoPtr nsWinGesture::getGestureInfo = nsnull;
nsWinGesture::CloseGestureInfoHandlePtr nsWinGesture::closeGestureInfoHandle = nsnull;
nsWinGesture::GetGestureExtraArgsPtr nsWinGesture::getGestureExtraArgs = nsnull;
nsWinGesture::SetGestureConfigPtr nsWinGesture::setGestureConfig = nsnull;
nsWinGesture::GetGestureConfigPtr nsWinGesture::getGestureConfig = nsnull;
nsWinGesture::BeginPanningFeedbackPtr nsWinGesture::beginPanningFeedback = nsnull;
nsWinGesture::EndPanningFeedbackPtr nsWinGesture::endPanningFeedback = nsnull;
nsWinGesture::UpdatePanningFeedbackPtr nsWinGesture::updatePanningFeedback = nsnull;

nsWinGesture::RegisterTouchWindowPtr nsWinGesture::registerTouchWindow = nsnull;
nsWinGesture::UnregisterTouchWindowPtr nsWinGesture::unregisterTouchWindow = nsnull;
nsWinGesture::GetTouchInputInfoPtr nsWinGesture::getTouchInputInfo = nsnull;
nsWinGesture::CloseTouchInputHandlePtr nsWinGesture::closeTouchInputHandle = nsnull;

static bool gEnableSingleFingerPanEvents = false;

nsWinGesture::nsWinGesture() :
  mPanActive(false),
  mFeedbackActive(false),
  mXAxisFeedback(false),
  mYAxisFeedback(false),
  mPanInertiaActive(false)
{
  (void)InitLibrary();
  mPixelScrollOverflow = 0;
}



bool nsWinGesture::InitLibrary()
{
  if (getGestureInfo) {
    return true;
  } else if (sLibraryHandle) {
    return false;
  }

  sLibraryHandle = ::LoadLibraryW(kGestureLibraryName);
  HMODULE hTheme = nsUXThemeData::GetThemeDLL();

  
  if (sLibraryHandle) {
    getGestureInfo = (GetGestureInfoPtr)GetProcAddress(sLibraryHandle, "GetGestureInfo");
    closeGestureInfoHandle = (CloseGestureInfoHandlePtr)GetProcAddress(sLibraryHandle, "CloseGestureInfoHandle");
    getGestureExtraArgs = (GetGestureExtraArgsPtr)GetProcAddress(sLibraryHandle, "GetGestureExtraArgs");
    setGestureConfig = (SetGestureConfigPtr)GetProcAddress(sLibraryHandle, "SetGestureConfig");
    getGestureConfig = (GetGestureConfigPtr)GetProcAddress(sLibraryHandle, "GetGestureConfig");
    registerTouchWindow = (RegisterTouchWindowPtr)GetProcAddress(sLibraryHandle, "RegisterTouchWindow");
    unregisterTouchWindow = (UnregisterTouchWindowPtr)GetProcAddress(sLibraryHandle, "UnregisterTouchWindow");
    getTouchInputInfo = (GetTouchInputInfoPtr)GetProcAddress(sLibraryHandle, "GetTouchInputInfo");
    closeTouchInputHandle = (CloseTouchInputHandlePtr)GetProcAddress(sLibraryHandle, "CloseTouchInputHandle");
  }

  if (!getGestureInfo || !closeGestureInfoHandle || !getGestureExtraArgs ||
    !setGestureConfig || !getGestureConfig) {
    getGestureInfo         = nsnull;
    closeGestureInfoHandle = nsnull;
    getGestureExtraArgs    = nsnull;
    setGestureConfig       = nsnull;
    getGestureConfig       = nsnull;
    return false;
  }
  
  if (!registerTouchWindow || !unregisterTouchWindow || !getTouchInputInfo || !closeTouchInputHandle) {
    registerTouchWindow   = nsnull;
    unregisterTouchWindow = nsnull;
    getTouchInputInfo     = nsnull;
    closeTouchInputHandle = nsnull;
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

  
  
  gEnableSingleFingerPanEvents =
    Preferences::GetBool("gestures.enable_single_finger_input", false);

  return true;
}

#define GCOUNT 5

bool nsWinGesture::SetWinGestureSupport(HWND hWnd, nsGestureNotifyEvent::ePanDirection aDirection)
{
  if (!getGestureInfo)
    return false;

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



bool nsWinGesture::IsAvailable()
{
  return getGestureInfo != nsnull;
}

bool nsWinGesture::RegisterTouchWindow(HWND hWnd)
{
  if (!registerTouchWindow)
    return false;

  return registerTouchWindow(hWnd, TWF_WANTPALM);
}

bool nsWinGesture::UnregisterTouchWindow(HWND hWnd)
{
  if (!unregisterTouchWindow)
    return false;

  return unregisterTouchWindow(hWnd);
}

bool nsWinGesture::GetTouchInputInfo(HTOUCHINPUT hTouchInput, PRUint32 cInputs, PTOUCHINPUT pInputs)
{
  if (!getTouchInputInfo)
    return false;

  return getTouchInputInfo(hTouchInput, cInputs, pInputs, sizeof(TOUCHINPUT));
}

bool nsWinGesture::CloseTouchInputHandle(HTOUCHINPUT hTouchInput)
{
  if (!closeTouchInputHandle)
    return false;

  return closeTouchInputHandle(hTouchInput);
}

bool nsWinGesture::GetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo)
{
  if (!getGestureInfo || !hGestureInfo || !pGestureInfo)
    return false;

  ZeroMemory(pGestureInfo, sizeof(GESTUREINFO));
  pGestureInfo->cbSize = sizeof(GESTUREINFO);

  return getGestureInfo(hGestureInfo, pGestureInfo);
}

bool nsWinGesture::CloseGestureInfoHandle(HGESTUREINFO hGestureInfo)
{
  if (!getGestureInfo || !hGestureInfo)
    return false;

  return closeGestureInfoHandle(hGestureInfo);
}

bool nsWinGesture::GetGestureExtraArgs(HGESTUREINFO hGestureInfo, UINT cbExtraArgs, PBYTE pExtraArgs)
{
  if (!getGestureInfo || !hGestureInfo || !pExtraArgs)
    return false;

  return getGestureExtraArgs(hGestureInfo, cbExtraArgs, pExtraArgs);
}

bool nsWinGesture::SetGestureConfig(HWND hWnd, UINT cIDs, PGESTURECONFIG pGestureConfig)
{
  if (!getGestureInfo || !pGestureConfig)
    return false;

  return setGestureConfig(hWnd, 0, cIDs, pGestureConfig, sizeof(GESTURECONFIG));
}

bool nsWinGesture::GetGestureConfig(HWND hWnd, DWORD dwFlags, PUINT pcIDs, PGESTURECONFIG pGestureConfig)
{
  if (!getGestureInfo || !pGestureConfig)
    return false;

  return getGestureConfig(hWnd, 0, dwFlags, pcIDs, pGestureConfig, sizeof(GESTURECONFIG));
}

bool nsWinGesture::BeginPanningFeedback(HWND hWnd)
{
  if (!beginPanningFeedback)
    return false;

  return beginPanningFeedback(hWnd);
}

bool nsWinGesture::EndPanningFeedback(HWND hWnd)
{
  if (!beginPanningFeedback)
    return false;

  return endPanningFeedback(hWnd, TRUE);
}

bool nsWinGesture::UpdatePanningFeedback(HWND hWnd, LONG offsetX, LONG offsetY, BOOL fInInertia)
{
  if (!beginPanningFeedback)
    return false;

  return updatePanningFeedback(hWnd, offsetX, offsetY, fInInertia);
}

bool nsWinGesture::IsPanEvent(LPARAM lParam)
{
  GESTUREINFO gi;

  ZeroMemory(&gi,sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);

  BOOL result = GetGestureInfo((HGESTUREINFO)lParam, &gi);
  if (!result)
    return false;

  if (gi.dwID == GID_PAN)
    return true;

  return false;
}



bool
nsWinGesture::ProcessGestureMessage(HWND hWnd, WPARAM wParam, LPARAM lParam, nsSimpleGestureEvent& evt)
{
  GESTUREINFO gi;

  ZeroMemory(&gi,sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);

  BOOL result = GetGestureInfo((HGESTUREINFO)lParam, &gi);
  if (!result)
    return false;

  
  nsPointWin coord;
  coord = gi.ptsLocation;
  coord.ScreenToClient(hWnd);

  evt.refPoint.x = coord.x;
  evt.refPoint.y = coord.y;

  
  
  switch(gi.dwID)
  {
    case GID_BEGIN:
    case GID_END:
      
      return false;
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

  return true;
}

bool
nsWinGesture::ProcessPanMessage(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  GESTUREINFO gi;

  ZeroMemory(&gi,sizeof(GESTUREINFO));
  gi.cbSize = sizeof(GESTUREINFO);

  BOOL result = GetGestureInfo((HGESTUREINFO)lParam, &gi);
  if (!result)
    return false;

  
  nsPointWin coord;
  coord = mPanRefPoint = gi.ptsLocation;
  
  
  mPanRefPoint.ScreenToClient(hWnd);

  switch(gi.dwID)
  {
    case GID_BEGIN:
    case GID_END:
      
      return false;
      break;

    
    case GID_PAN:
    {
      if (gi.dwFlags & GF_BEGIN) {
        mPanIntermediate = coord;
        mPixelScrollDelta = 0;
        mPanActive = true;
        mPanInertiaActive = false;
      }
      else {

#ifdef DBG_jimm
        PRInt32 deltaX = mPanIntermediate.x - coord.x;
        PRInt32 deltaY = mPanIntermediate.y - coord.y;
        PR_LOG(gWindowsLog, PR_LOG_ALWAYS, 
               ("coordX=%d coordY=%d deltaX=%d deltaY=%d x:%d y:%d\n", coord.x,
                coord.y, deltaX, deltaY, mXAxisFeedback, mYAxisFeedback));
#endif

        mPixelScrollDelta.x = mPanIntermediate.x - coord.x;
        mPixelScrollDelta.y = mPanIntermediate.y - coord.y;
        mPanIntermediate = coord;

        if (gi.dwFlags & GF_INERTIA)
          mPanInertiaActive = true;

        if (gi.dwFlags & GF_END) {
          mPanActive = false;
          mPanInertiaActive = false;
          PanFeedbackFinalize(hWnd, true);
        }
      }
    }
    break;
  }
  return true;
}

inline bool TestTransition(PRInt32 a, PRInt32 b)
{
  
  
  if (a == 0 || b == 0) return true;
  
  return (a < 0) == (b < 0);
}

void
nsWinGesture::UpdatePanFeedbackX(HWND hWnd, PRInt32 scrollOverflow, bool& endFeedback)
{
  
  
  if (scrollOverflow != 0) {
    if (!mFeedbackActive) {
      BeginPanningFeedback(hWnd);
      mFeedbackActive = true;
    }      
    endFeedback = false;
    mXAxisFeedback = true;
    return;
  }
  
  if (mXAxisFeedback) {
    PRInt32 newOverflow = mPixelScrollOverflow.x - mPixelScrollDelta.x;

    
    
    if (!TestTransition(newOverflow, mPixelScrollOverflow.x) || newOverflow == 0)
      return;

    
    mPixelScrollOverflow.x = newOverflow;
    endFeedback = false;
  }
}

void
nsWinGesture::UpdatePanFeedbackY(HWND hWnd, PRInt32 scrollOverflow, bool& endFeedback)
{
  
  
  if (scrollOverflow != 0) {
    if (!mFeedbackActive) {
      BeginPanningFeedback(hWnd);
      mFeedbackActive = true;
    }
    endFeedback = false;
    mYAxisFeedback = true;
    return;
  }
  
  if (mYAxisFeedback) {
    PRInt32 newOverflow = mPixelScrollOverflow.y - mPixelScrollDelta.y;

    
    
    if (!TestTransition(newOverflow, mPixelScrollOverflow.y) || newOverflow == 0)
      return;

    
    mPixelScrollOverflow.y = newOverflow;
    endFeedback = false;
  }
}

void
nsWinGesture::PanFeedbackFinalize(HWND hWnd, bool endFeedback)
{
  if (!mFeedbackActive)
    return;

  if (endFeedback) {
    mFeedbackActive = false;
    mXAxisFeedback = false;
    mYAxisFeedback = false;
    mPixelScrollOverflow = 0;
    EndPanningFeedback(hWnd);
    return;
  }

  UpdatePanningFeedback(hWnd, mPixelScrollOverflow.x, mPixelScrollOverflow.y, mPanInertiaActive);
}

bool
nsWinGesture::PanDeltaToPixelScrollX(nsMouseScrollEvent& evt)
{
  evt.delta = 0;
  evt.scrollOverflow = 0;

  
  
  
  if (mXAxisFeedback)
    return false;

  if (mPixelScrollDelta.x != 0)
  {
    evt.scrollFlags = nsMouseScrollEvent::kIsHorizontal|
                      nsMouseScrollEvent::kHasPixels|
                      nsMouseScrollEvent::kNoLines|
                      nsMouseScrollEvent::kNoDefer;
    evt.delta = mPixelScrollDelta.x;
    evt.refPoint.x = mPanRefPoint.x;
    evt.refPoint.y = mPanRefPoint.y;
    return true;
  }
  return false;
}

bool
nsWinGesture::PanDeltaToPixelScrollY(nsMouseScrollEvent& evt)
{
  evt.delta = 0;
  evt.scrollOverflow = 0;

  
  
  
  if (mYAxisFeedback)
    return false;

  if (mPixelScrollDelta.y != 0)
  {
    evt.scrollFlags = nsMouseScrollEvent::kIsVertical|
                      nsMouseScrollEvent::kHasPixels|
                      nsMouseScrollEvent::kNoLines|
                      nsMouseScrollEvent::kNoDefer;
    evt.delta = mPixelScrollDelta.y;
    evt.refPoint.x = mPanRefPoint.x;
    evt.refPoint.y = mPanRefPoint.y;
    return true;
  }
  return false;
}
