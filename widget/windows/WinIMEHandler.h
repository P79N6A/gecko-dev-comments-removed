




#ifndef WinIMEHandler_h_
#define WinIMEHandler_h_

#include "nscore.h"
#include <windows.h>

class nsWindow;

namespace mozilla {
namespace widget {







class IMEHandler MOZ_FINAL
{
public:
  static void Initialize();
  static void Terminate();

  





  static bool ProcessMessage(nsWindow* aWindow, UINT aMessage,
                             WPARAM& aWParam, LPARAM& aLParam,
                             LRESULT* aRetValue, bool& aEatMessage);

  




  static bool IsDoingKakuteiUndo(HWND aWnd);

private:
#ifdef NS_ENABLE_TSF
  static bool sIsInTSFMode;
#endif 
};

} 
} 

#endif 
