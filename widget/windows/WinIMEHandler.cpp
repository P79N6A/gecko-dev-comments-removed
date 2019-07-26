




#include "WinIMEHandler.h"
#include "nsIMM32Handler.h"

#ifdef NS_ENABLE_TSF
#include "nsTextStore.h"
#endif 

#include "nsWindow.h"

namespace mozilla {
namespace widget {





#ifdef NS_ENABLE_TSF
bool IMEHandler::sIsInTSFMode = false;
#endif 


void
IMEHandler::Initialize()
{
#ifdef NS_ENABLE_TSF
  nsTextStore::Initialize();
  sIsInTSFMode = nsTextStore::IsInTSFMode();
#endif 

  nsIMM32Handler::Initialize();
}


void
IMEHandler::Terminate()
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    nsTextStore::Terminate();
    sIsInTSFMode = false;
  }
#endif 

  nsIMM32Handler::Terminate();
}


bool
IMEHandler::ProcessMessage(nsWindow* aWindow, UINT aMessage,
                           WPARAM& aWParam, LPARAM& aLParam,
                           LRESULT* aRetValue, bool& aEatMessage)
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    if (aMessage == WM_USER_TSF_TEXTCHANGE) {
      nsTextStore::OnTextChangeMsg();
      aEatMessage = true;
      return true;
    }
    return false;
  }
#endif 

  return nsIMM32Handler::ProcessMessage(aWindow, aMessage, aWParam, aLParam,
                                        aRetValue, aEatMessage);
}


bool
IMEHandler::IsComposing()
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    return nsTextStore::IsComposing();
  }
#endif 

  return nsIMM32Handler::IsComposing();
}


bool
IMEHandler::IsComposingOn(nsWindow* aWindow)
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    return nsTextStore::IsComposingOn(aWindow);
  }
#endif 

  return nsIMM32Handler::IsComposingOn(aWindow);
}


nsresult
IMEHandler::NotifyIME(nsWindow* aWindow,
                      NotificationToIME aNotification)
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    switch (aNotification) {
      case NOTIFY_IME_OF_SELECTION_CHANGE:
        return nsTextStore::OnSelectionChange();
      case NOTIFY_IME_OF_FOCUS:
        return nsTextStore::OnFocusChange(true, aWindow,
                 aWindow->GetInputContext().mIMEState.mEnabled);
      case NOTIFY_IME_OF_BLUR:
        return nsTextStore::OnFocusChange(false, aWindow,
                 aWindow->GetInputContext().mIMEState.mEnabled);
      case REQUEST_TO_COMMIT_COMPOSITION:
        if (nsTextStore::IsComposingOn(aWindow)) {
          nsTextStore::CommitComposition(false);
        }
        return NS_OK;
      case REQUEST_TO_CANCEL_COMPOSITION:
        if (nsTextStore::IsComposingOn(aWindow)) {
          nsTextStore::CommitComposition(true);
        }
        return NS_OK;
      default:
        return NS_ERROR_NOT_IMPLEMENTED;
    }
  }
#endif 

  switch (aNotification) {
    case REQUEST_TO_COMMIT_COMPOSITION:
      nsIMM32Handler::CommitComposition(aWindow);
      return NS_OK;
    case REQUEST_TO_CANCEL_COMPOSITION:
      nsIMM32Handler::CancelComposition(aWindow);
      return NS_OK;
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
}


nsresult
IMEHandler::NotifyIMEOfTextChange(uint32_t aStart,
                                  uint32_t aOldEnd,
                                  uint32_t aNewEnd)
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    return nsTextStore::OnTextChange(aStart, aOldEnd, aNewEnd);
  }
#endif 

  return NS_ERROR_NOT_IMPLEMENTED;
}


nsIMEUpdatePreference
IMEHandler::GetUpdatePreference()
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    return nsTextStore::GetIMEUpdatePreference();
  }
#endif 

  return nsIMEUpdatePreference(false, false);
}

void
IMEHandler::SetOpenState(nsWindow* aWindow, bool aOpen)
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    nsTextStore::SetIMEOpenState(aOpen);
    return;
  }
#endif 

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  IMEContext.SetOpenState(aOpen);
}


bool
IMEHandler::GetOpenState(nsWindow* aWindow)
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    return nsTextStore::GetIMEOpenState();
  }
#endif 

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  return IMEContext.GetOpenState();
}

#ifdef DEBUG

bool
IMEHandler::CurrentKeyboardLayoutHasIME()
{
#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    return nsTextStore::CurrentKeyboardLayoutHasIME();
  }
#endif 

  return nsIMM32Handler::IsIMEAvailable();
}
#endif 


bool
IMEHandler::IsDoingKakuteiUndo(HWND aWnd)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  MSG startCompositionMsg, compositionMsg, charMsg;
  return ::PeekMessageW(&startCompositionMsg, aWnd,
                        WM_IME_STARTCOMPOSITION, WM_IME_STARTCOMPOSITION,
                        PM_NOREMOVE | PM_NOYIELD) &&
         ::PeekMessageW(&compositionMsg, aWnd, WM_IME_COMPOSITION,
                        WM_IME_COMPOSITION, PM_NOREMOVE | PM_NOYIELD) &&
         ::PeekMessageW(&charMsg, aWnd, WM_CHAR, WM_CHAR,
                        PM_NOREMOVE | PM_NOYIELD) &&
         startCompositionMsg.wParam == 0x0 &&
         startCompositionMsg.lParam == 0x0 &&
         compositionMsg.wParam == 0x0 &&
         compositionMsg.lParam == 0x1BF &&
         charMsg.wParam == VK_BACK && charMsg.lParam == 0x1 &&
         startCompositionMsg.time <= compositionMsg.time &&
         compositionMsg.time <= charMsg.time;
}

} 
} 
