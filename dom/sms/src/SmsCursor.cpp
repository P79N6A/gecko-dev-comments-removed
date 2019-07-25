




































#include "SmsCursor.h"
#include "nsIDOMClassInfo.h"
#include "nsDOMError.h"
#include "nsIDOMSmsFilter.h"
#include "nsIDOMSmsMessage.h"

DOMCI_DATA(MozSmsCursor, mozilla::dom::sms::SmsCursor)

namespace mozilla {
namespace dom {
namespace sms {

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SmsCursor)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsCursor)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsCursor)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_2(SmsCursor, mFilter, mMessage)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SmsCursor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SmsCursor)

SmsCursor::SmsCursor(nsIDOMMozSmsFilter* aFilter)
  : mFilter(aFilter)
{
}

NS_IMETHODIMP
SmsCursor::GetFilter(nsIDOMMozSmsFilter** aFilter)
{
  NS_ADDREF(*aFilter = mFilter);
  return NS_OK;
}

NS_IMETHODIMP
SmsCursor::GetMessage(nsIDOMMozSmsMessage** aMessage)
{
  
  *aMessage = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
SmsCursor::Continue()
{
  
  if (!mMessage) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  
  
  

  return NS_OK;
}

} 
} 
} 

