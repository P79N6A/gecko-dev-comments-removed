



#include "nsIMEPicker.h"
#include "AndroidBridge.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsIMEPicker, nsIIMEPicker)

nsIMEPicker::nsIMEPicker()
{
  
}

nsIMEPicker::~nsIMEPicker()
{
  
}


NS_IMETHODIMP nsIMEPicker::Show()
{
    mozilla::widget::android::GeckoAppShell::ShowInputMethodPicker();
    return NS_OK;
}
