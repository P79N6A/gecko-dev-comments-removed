




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







class IMEHandler MOZ_FINAL
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
                            NotificationToIME aNotification);

  


  static nsresult NotifyIMEOfTextChange(uint32_t aStart,
                                        uint32_t aOldEnd,
                                        uint32_t aNewEnd);

  


  static nsIMEUpdatePreference GetUpdatePreference();

  


  static bool GetOpenState(nsWindow* aWindow);

  


  static void OnDestroyWindow(nsWindow* aWindow);

  



  static void SetInputContext(nsWindow* aWindow,
                              InputContext& aInputContext,
                              const InputContextAction& aAction);

  


  static void InitInputContext(nsWindow* aWindow, InputContext& aInputContext);

#ifdef DEBUG
  


  static bool CurrentKeyboardLayoutHasIME();
#endif 

private:
#ifdef NS_ENABLE_TSF
  typedef HRESULT (WINAPI *SetInputScopesFunc)(HWND windowHandle,
                                               const InputScope *inputScopes,
                                               UINT numInputScopes,
                                               wchar_t **phrase_list,
                                               UINT numPhraseList,
                                               wchar_t *regExp,
                                               wchar_t *srgs);
  static SetInputScopesFunc sSetInputScopes;
  static void SetInputScopeForIMM32(nsWindow* aWindow,
                                    const nsAString& aHTMLInputType);
  static bool sIsInTSFMode;
  static bool sPluginHasFocus;

  static bool IsTSFAvailable() { return (sIsInTSFMode && !sPluginHasFocus); }
#endif 
};

} 
} 

#endif 
