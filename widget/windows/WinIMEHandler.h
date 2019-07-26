




#ifndef WinIMEHandler_h_
#define WinIMEHandler_h_

#include "nscore.h"
#include <windows.h>

namespace mozilla {
namespace widget {







class IMEHandler MOZ_FINAL
{
public:
  static void Initialize();
  static void Terminate();

  




  static bool IsDoingKakuteiUndo(HWND aWnd);

private:
#ifdef NS_ENABLE_TSF
  static bool sIsInTSFMode;
#endif 
};

} 
} 

#endif 
