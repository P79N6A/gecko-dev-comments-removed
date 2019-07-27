




#ifndef WinIMEHandler_h_
#define WinIMEHandler_h_

#include "nscore.h"
#include "nsIWidget.h"
#include <windows.h>
#include <inputscope.h>

#define NS_WM_IMEFIRST WM_IME_SETCONTEXT
#define NS_WM_IMELAST  WM_IME_KEYUP

class nsWindow;

namespace mozilla {
namespace widget {

struct MSGResult;







class IMEHandler final
{
public:
  static void Initialize();
  static void Terminate();

  


  static void* GetNativeData(uint32_t aDataType);

  




  static bool ProcessRawKeyMessage(const MSG& aMsg);

  



  static bool ProcessMessage(nsWindow* aWindow, UINT aMessage,
                             WPARAM& aWParam, LPARAM& aLParam,
                             MSGResult& aResult);

  


  static bool IsComposing();

  



  static bool IsComposingOn(nsWindow* aWindow);

  


  static nsresult NotifyIME(nsWindow* aWindow,
                            const IMENotification& aIMENotification);

  


  static nsIMEUpdatePreference GetUpdatePreference();

  


  static bool GetOpenState(nsWindow* aWindow);

  


  static void OnDestroyWindow(nsWindow* aWindow);

  



  static void SetInputContext(nsWindow* aWindow,
                              InputContext& aInputContext,
                              const InputContextAction& aAction);

  


  static void AssociateIMEContext(nsWindow* aWindow, bool aEnable);

  


  static void InitInputContext(nsWindow* aWindow, InputContext& aInputContext);

#ifdef DEBUG
  


  static bool CurrentKeyboardLayoutHasIME();
#endif 

private:
#ifdef NS_ENABLE_TSF
  static decltype(SetInputScopes)* sSetInputScopes;
  static void SetInputScopeForIMM32(nsWindow* aWindow,
                                    const nsAString& aHTMLInputType);
  static bool sIsInTSFMode;
  
  
  static bool sIsIMMEnabled;
  static bool sPluginHasFocus;

  static bool IsTSFAvailable() { return (sIsInTSFMode && !sPluginHasFocus); }
  static bool IsIMMActive();
#endif 
};

} 
} 

#endif 
