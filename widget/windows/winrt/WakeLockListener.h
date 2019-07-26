



#include "mozwrlbase.h"

#include "nscore.h"
#include "nsString.h"
#include "nsIDOMWakeLockListener.h"

#include <windows.system.display.h>





class WakeLockListener :
  public nsIDOMMozWakeLockListener {
public:
  NS_DECL_ISUPPORTS;
  NS_DECL_NSIDOMMOZWAKELOCKLISTENER;

private:
  Microsoft::WRL::ComPtr<ABI::Windows::System::Display::IDisplayRequest> mDisplayRequest;
};
