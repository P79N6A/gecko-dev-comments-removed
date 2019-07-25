




































#include "SmsManager.h"
#include "nsIDOMClassInfo.h"

DOMCI_DATA(MozSmsManager, mozilla::dom::sms::SmsManager)

namespace mozilla {
namespace dom {
namespace sms {

NS_INTERFACE_MAP_BEGIN(SmsManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozSmsManager)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozSmsManager)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(SmsManager)
NS_IMPL_RELEASE(SmsManager)

} 
} 
} 
