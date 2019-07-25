




































#include "Connection.h"
#include "nsIDOMClassInfo.h"
#include "mozilla/Preferences.h"

DOMCI_DATA(MozConnection, mozilla::dom::network::Connection)

namespace mozilla {
namespace dom {
namespace network {

const char* Connection::sMeteredPrefName     = "dom.network.metered";
const bool  Connection::sMeteredDefaultValue = false;

NS_INTERFACE_MAP_BEGIN(Connection)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozConnection)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozConnection)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(Connection)
NS_IMPL_RELEASE(Connection)

NS_IMETHODIMP
Connection::GetMetered(bool* aMetered)
{
  *aMetered = Preferences::GetBool(sMeteredPrefName,
                                   sMeteredDefaultValue);
  return NS_OK;
}

} 
} 
} 

