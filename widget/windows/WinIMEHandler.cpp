




#include "WinIMEHandler.h"
#include "nsIMM32Handler.h"

#ifdef NS_ENABLE_TSF
#include "nsTextStore.h"
#endif 

#include "nsWindow.h"
#include "WinUtils.h"

namespace mozilla {
namespace widget {





#ifdef NS_ENABLE_TSF
bool IMEHandler::sIsInTSFMode = false;
bool IMEHandler::sPluginHasFocus = false;
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


void*
IMEHandler::GetNativeData(uint32_t aDataType)
{
#ifdef NS_ENABLE_TSF
  void* result = nsTextStore::GetNativeData(aDataType);
  if (!result || !(*(static_cast<void**>(result)))) {
    return nullptr;
  }
  
  
  
  
  sIsInTSFMode = true;
  return result;
#else 
  return nullptr;
#endif 
}


bool
IMEHandler::IsIMEEnabled(const InputContext& aInputContext)
{
  return IsIMEEnabled(aInputContext.mIMEState.mEnabled);
}


bool
IMEHandler::IsIMEEnabled(IMEState::Enabled aIMEState)
{
  return (aIMEState == mozilla::widget::IMEState::ENABLED ||
          aIMEState == mozilla::widget::IMEState::PLUGIN);
}


bool
IMEHandler::ProcessRawKeyMessage(const MSG& aMsg)
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
    return nsTextStore::ProcessRawKeyMessage(aMsg);
  }
#endif 
  return false; 
}


bool
IMEHandler::ProcessMessage(nsWindow* aWindow, UINT aMessage,
                           WPARAM& aWParam, LPARAM& aLParam,
                           LRESULT* aRetValue, bool& aEatMessage)
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
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
  if (IsTSFAvailable()) {
    return nsTextStore::IsComposing();
  }
#endif 

  return nsIMM32Handler::IsComposing();
}


bool
IMEHandler::IsComposingOn(nsWindow* aWindow)
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
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
  if (IsTSFAvailable()) {
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
#ifdef NS_ENABLE_TSF
    case NOTIFY_IME_OF_BLUR:
      
      
      if (nsTextStore::ThinksHavingFocus()) {
        return nsTextStore::OnFocusChange(false, aWindow,
                 aWindow->GetInputContext().mIMEState.mEnabled);
      }
      return NS_ERROR_NOT_IMPLEMENTED;
#endif 
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
  if (IsTSFAvailable()) {
    return nsTextStore::OnTextChange(aStart, aOldEnd, aNewEnd);
  }
#endif 

  return NS_ERROR_NOT_IMPLEMENTED;
}


nsIMEUpdatePreference
IMEHandler::GetUpdatePreference()
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
    return nsTextStore::GetIMEUpdatePreference();
  }
#endif 

  return nsIMEUpdatePreference(false, false);
}


bool
IMEHandler::GetOpenState(nsWindow* aWindow)
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
    return nsTextStore::GetIMEOpenState();
  }
#endif 

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  return IMEContext.GetOpenState();
}


void
IMEHandler::OnDestroyWindow(nsWindow* aWindow)
{
  
  
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  IMEContext.AssociateDefaultContext();
}


void
IMEHandler::SetInputContext(nsWindow* aWindow,
                            InputContext& aInputContext,
                            const InputContextAction& aAction)
{
  
  NotifyIME(aWindow, REQUEST_TO_COMMIT_COMPOSITION);

  
  sPluginHasFocus = (aInputContext.mIMEState.mEnabled == IMEState::PLUGIN);

  bool enable = IsIMEEnabled(aInputContext);
  bool adjustOpenState = (enable &&
    aInputContext.mIMEState.mOpen != IMEState::DONT_CHANGE_OPEN_STATE);
  bool open = (adjustOpenState &&
    aInputContext.mIMEState.mOpen == IMEState::OPEN);

  aInputContext.mNativeIMEContext = nullptr;

#ifdef NS_ENABLE_TSF
  
  if (sIsInTSFMode) {
    nsTextStore::SetInputContext(aWindow, aInputContext, aAction);
    if (IsTSFAvailable()) {
      aInputContext.mNativeIMEContext = nsTextStore::GetTextStore();
      if (adjustOpenState) {
        nsTextStore::SetIMEOpenState(open);
      }
      return;
    }
  }
#endif 

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  if (enable) {
    IMEContext.AssociateDefaultContext();
    if (!aInputContext.mNativeIMEContext) {
      aInputContext.mNativeIMEContext = static_cast<void*>(IMEContext.get());
    }
  } else if (!aWindow->Destroyed()) {
    
    IMEContext.Disassociate();
    if (!aInputContext.mNativeIMEContext) {
      
      aInputContext.mNativeIMEContext =
        aWindow->GetInputContext().mNativeIMEContext;
    }
  }

  if (adjustOpenState) {
    IMEContext.SetOpenState(open);
  }
}


void
IMEHandler::InitInputContext(nsWindow* aWindow, InputContext& aInputContext)
{
  
  aInputContext.mIMEState.mEnabled = IMEState::ENABLED;

#ifdef NS_ENABLE_TSF
  if (sIsInTSFMode) {
    nsTextStore::SetInputContext(aWindow, aInputContext,
      InputContextAction(InputContextAction::CAUSE_UNKNOWN,
                         InputContextAction::GOT_FOCUS));
    aInputContext.mNativeIMEContext = nsTextStore::GetTextStore();
    MOZ_ASSERT(aInputContext.mNativeIMEContext);
    return;
  }
#endif 

  
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  aInputContext.mNativeIMEContext = static_cast<void*>(IMEContext.get());
  MOZ_ASSERT(aInputContext.mNativeIMEContext || !CurrentKeyboardLayoutHasIME());
  
  
  if (!aInputContext.mNativeIMEContext) {
    aInputContext.mNativeIMEContext = static_cast<void*>(aWindow);
  }
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
  return WinUtils::PeekMessage(&startCompositionMsg, aWnd,
                               WM_IME_STARTCOMPOSITION, WM_IME_STARTCOMPOSITION,
                               PM_NOREMOVE | PM_NOYIELD) &&
         WinUtils::PeekMessage(&compositionMsg, aWnd, WM_IME_COMPOSITION,
                               WM_IME_COMPOSITION, PM_NOREMOVE | PM_NOYIELD) &&
         WinUtils::PeekMessage(&charMsg, aWnd, WM_CHAR, WM_CHAR,
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
