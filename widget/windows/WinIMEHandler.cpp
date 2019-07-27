




#include "WinIMEHandler.h"

#include "mozilla/Preferences.h"
#include "nsIMM32Handler.h"
#include "nsWindowDefs.h"

#ifdef NS_ENABLE_TSF
#include "nsTextStore.h"
#endif 

#include "nsWindow.h"
#include "WinUtils.h"

namespace mozilla {
namespace widget {





#ifdef NS_ENABLE_TSF
bool IMEHandler::sIsInTSFMode = false;
bool IMEHandler::sIsIMMEnabled = true;
bool IMEHandler::sPluginHasFocus = false;
decltype(SetInputScopes)* IMEHandler::sSetInputScopes = nullptr;
#endif 


void
IMEHandler::Initialize()
{
#ifdef NS_ENABLE_TSF
  nsTextStore::Initialize();
  sIsInTSFMode = nsTextStore::IsInTSFMode();
  sIsIMMEnabled =
    !sIsInTSFMode || Preferences::GetBool("intl.tsf.support_imm", true);
  if (!sIsInTSFMode) {
    
    
    
    HMODULE module = nullptr;
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"msctf.dll",
                           &module)) {
      sSetInputScopes = reinterpret_cast<decltype(SetInputScopes)*>(
        GetProcAddress(module, "SetInputScopes"));
    }
  }
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
                           MSGResult& aResult)
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
    nsTextStore::ProcessMessage(aWindow, aMessage, aWParam, aLParam, aResult);
    if (aResult.mConsumed) {
      return true;
    }
    
    if (!sIsIMMEnabled) {
      return false;
    }
    
    
    if (!nsTextStore::IsIMM_IME()) {
      return false;
    }
  }
#endif 

  return nsIMM32Handler::ProcessMessage(aWindow, aMessage, aWParam, aLParam,
                                        aResult);
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
                      const IMENotification& aIMENotification)
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
    switch (aIMENotification.mMessage) {
      case NOTIFY_IME_OF_SELECTION_CHANGE:
        return nsTextStore::OnSelectionChange();
      case NOTIFY_IME_OF_TEXT_CHANGE:
        return nsTextStore::OnTextChange(aIMENotification);
      case NOTIFY_IME_OF_FOCUS:
        return nsTextStore::OnFocusChange(true, aWindow,
                 aWindow->GetInputContext().mIMEState);
      case NOTIFY_IME_OF_BLUR:
        return nsTextStore::OnFocusChange(false, aWindow,
                 aWindow->GetInputContext().mIMEState);
      case NOTIFY_IME_OF_MOUSE_BUTTON_EVENT:
        return nsTextStore::OnMouseButtonEvent(aIMENotification);
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
      case NOTIFY_IME_OF_POSITION_CHANGE:
        return nsTextStore::OnLayoutChange();
      default:
        return NS_ERROR_NOT_IMPLEMENTED;
    }
  }
#endif 

  switch (aIMENotification.mMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION:
      nsIMM32Handler::CommitComposition(aWindow);
      return NS_OK;
    case REQUEST_TO_CANCEL_COMPOSITION:
      nsIMM32Handler::CancelComposition(aWindow);
      return NS_OK;
    case NOTIFY_IME_OF_POSITION_CHANGE:
    case NOTIFY_IME_OF_COMPOSITION_UPDATE:
      nsIMM32Handler::OnUpdateComposition(aWindow);
      return NS_OK;
#ifdef NS_ENABLE_TSF
    case NOTIFY_IME_OF_BLUR:
      
      
      if (nsTextStore::ThinksHavingFocus()) {
        return nsTextStore::OnFocusChange(false, aWindow,
                                          aWindow->GetInputContext().mIMEState);
      }
      return NS_ERROR_NOT_IMPLEMENTED;
#endif 
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
}


nsIMEUpdatePreference
IMEHandler::GetUpdatePreference()
{
#ifdef NS_ENABLE_TSF
  if (IsTSFAvailable()) {
    return nsTextStore::GetIMEUpdatePreference();
  }
#endif 

  return nsIMM32Handler::GetIMEUpdatePreference();
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
#ifdef NS_ENABLE_TSF
  
  
  if (!sIsInTSFMode) {
    
    
    SetInputScopeForIMM32(aWindow, EmptyString());
  }
#endif 
  AssociateIMEContext(aWindow, true);
}


void
IMEHandler::SetInputContext(nsWindow* aWindow,
                            InputContext& aInputContext,
                            const InputContextAction& aAction)
{
  
  NotifyIME(aWindow, IMENotification(REQUEST_TO_COMMIT_COMPOSITION));

  const InputContext& oldInputContext = aWindow->GetInputContext();

  
  sPluginHasFocus = (aInputContext.mIMEState.mEnabled == IMEState::PLUGIN);

  bool enable = WinUtils::IsIMEEnabled(aInputContext);
  bool adjustOpenState = (enable &&
    aInputContext.mIMEState.mOpen != IMEState::DONT_CHANGE_OPEN_STATE);
  bool open = (adjustOpenState &&
    aInputContext.mIMEState.mOpen == IMEState::OPEN);

  aInputContext.mNativeIMEContext = nullptr;

#ifdef NS_ENABLE_TSF
  
  if (sIsInTSFMode) {
    nsTextStore::SetInputContext(aWindow, aInputContext, aAction);
    if (IsTSFAvailable()) {
      aInputContext.mNativeIMEContext = nsTextStore::GetThreadManager();
      if (sIsIMMEnabled) {
        
        AssociateIMEContext(aWindow, enable);
      } else if (oldInputContext.mIMEState.mEnabled == IMEState::PLUGIN) {
        
        
        AssociateIMEContext(aWindow, false);
      }
      if (adjustOpenState) {
        nsTextStore::SetIMEOpenState(open);
      }
      return;
    }
  } else {
    
    SetInputScopeForIMM32(aWindow, aInputContext.mHTMLInputType);
  }
#endif 

  AssociateIMEContext(aWindow, enable);

  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  if (adjustOpenState) {
    IMEContext.SetOpenState(open);
  }

  if (aInputContext.mNativeIMEContext) {
    return;
  }

  
  
  aInputContext.mNativeIMEContext = enable ?
    static_cast<void*>(IMEContext.get()) : oldInputContext.mNativeIMEContext;
}


void
IMEHandler::AssociateIMEContext(nsWindow* aWindow, bool aEnable)
{
  nsIMEContext IMEContext(aWindow->GetWindowHandle());
  if (aEnable) {
    IMEContext.AssociateDefaultContext();
    return;
  }
  
  if (aWindow->Destroyed()) {
    return;
  }
  IMEContext.Disassociate();
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
    aInputContext.mNativeIMEContext = nsTextStore::GetThreadManager();
    MOZ_ASSERT(aInputContext.mNativeIMEContext);
    
    if (!sIsIMMEnabled) {
      AssociateIMEContext(aWindow, false);
    }
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


void
IMEHandler::SetInputScopeForIMM32(nsWindow* aWindow,
                                  const nsAString& aHTMLInputType)
{
  if (sIsInTSFMode || !sSetInputScopes || aWindow->Destroyed()) {
    return;
  }
  UINT arraySize = 0;
  const InputScope* scopes = nullptr;
  
  if (aHTMLInputType.IsEmpty() || aHTMLInputType.EqualsLiteral("text")) {
    static const InputScope inputScopes[] = { IS_DEFAULT };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("url")) {
    static const InputScope inputScopes[] = { IS_URL };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("search")) {
    static const InputScope inputScopes[] = { IS_SEARCH };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("email")) {
    static const InputScope inputScopes[] = { IS_EMAIL_SMTPEMAILADDRESS };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("password")) {
    static const InputScope inputScopes[] = { IS_PASSWORD };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("datetime") ||
             aHTMLInputType.EqualsLiteral("datetime-local")) {
    static const InputScope inputScopes[] = {
      IS_DATE_FULLDATE, IS_TIME_FULLTIME };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("date") ||
             aHTMLInputType.EqualsLiteral("month") ||
             aHTMLInputType.EqualsLiteral("week")) {
    static const InputScope inputScopes[] = { IS_DATE_FULLDATE };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("time")) {
    static const InputScope inputScopes[] = { IS_TIME_FULLTIME };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("tel")) {
    static const InputScope inputScopes[] = {
      IS_TELEPHONE_FULLTELEPHONENUMBER, IS_TELEPHONE_LOCALNUMBER };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  } else if (aHTMLInputType.EqualsLiteral("number")) {
    static const InputScope inputScopes[] = { IS_NUMBER };
    scopes = &inputScopes[0];
    arraySize = ArrayLength(inputScopes);
  }
  if (scopes && arraySize > 0) {
    sSetInputScopes(aWindow->GetWindowHandle(), scopes, arraySize, nullptr, 0,
                    nullptr, nullptr);
  }
}

} 
} 
