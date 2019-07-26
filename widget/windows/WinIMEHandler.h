




#ifndef WinIMEHandler_h_
#define WinIMEHandler_h_

#include "nscore.h"
#include "nsEvent.h"
#include "nsIWidget.h"
#include <windows.h>

class nsWindow;

namespace mozilla {
namespace widget {







class IMEHandler MOZ_FINAL
{
public:
  static void Initialize();
  static void Terminate();

  


  static bool IsIMEEnabled(const InputContext& aInputContext);
  static bool IsIMEEnabled(IMEState::Enabled aIMEState);

  





  static bool ProcessMessage(nsWindow* aWindow, UINT aMessage,
                             WPARAM& aWParam, LPARAM& aLParam,
                             LRESULT* aRetValue, bool& aEatMessage);

  


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

  



  static void SetInputContext(nsWindow* aWindow, InputContext& aInputContext);

  


  static void InitInputContext(nsWindow* aWindow, InputContext& aInputContext);

  




  static bool IsDoingKakuteiUndo(HWND aWnd);

#ifdef DEBUG
  


  static bool CurrentKeyboardLayoutHasIME();
#endif 

private:
#ifdef NS_ENABLE_TSF
  static bool sIsInTSFMode;
#endif 
};

} 
} 

#endif 
