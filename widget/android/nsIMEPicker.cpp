



#include "nsIMEPicker.h"
#include "AndroidBridge.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS(nsIMEPicker, nsIIMEPicker)

nsIMEPicker::nsIMEPicker()
{
  
}

nsIMEPicker::~nsIMEPicker()
{
  
}


NS_IMETHODIMP nsIMEPicker::Show()
{
    widget::GeckoAppShell::ShowInputMethodPicker();
    return NS_OK;
}
