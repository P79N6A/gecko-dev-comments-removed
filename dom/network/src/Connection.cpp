




#include <limits>
#include "mozilla/Hal.h"
#include "mozilla/dom/network/Connection.h"
#include "nsIDOMClassInfo.h"
#include "mozilla/Preferences.h"
#include "Constants.h"





#define CHANGE_EVENT_NAME NS_LITERAL_STRING("typechange")

namespace mozilla {
namespace dom {
namespace network {

NS_IMPL_QUERY_INTERFACE_INHERITED(Connection, DOMEventTargetHelper,
                                  nsINetworkProperties)



NS_IMPL_ADDREF_INHERITED(dom::network::Connection, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(dom::network::Connection, DOMEventTargetHelper)

Connection::Connection()
  : mType(static_cast<ConnectionType>(kDefaultType))
  , mIsWifi(kDefaultIsWifi)
  , mDHCPGateway(kDefaultDHCPGateway)
{
  SetIsDOMBinding();
}

void
Connection::Init(nsPIDOMWindow* aWindow)
{
  BindToOwner(aWindow);

  hal::RegisterNetworkObserver(this);

  hal::NetworkInformation networkInfo;
  hal::GetCurrentNetworkInformation(&networkInfo);

  UpdateFromNetworkInfo(networkInfo);
}

void
Connection::Shutdown()
{
  hal::UnregisterNetworkObserver(this);
}

NS_IMETHODIMP
Connection::GetIsWifi(bool *aIsWifi)
{
  *aIsWifi = mIsWifi;
  return NS_OK;
}

NS_IMETHODIMP
Connection::GetDhcpGateway(uint32_t *aGW)
{
  *aGW = mDHCPGateway;
  return NS_OK;
}

void
Connection::UpdateFromNetworkInfo(const hal::NetworkInformation& aNetworkInfo)
{
  mType = static_cast<ConnectionType>(aNetworkInfo.type());
  mIsWifi = aNetworkInfo.isWifi();
  mDHCPGateway = aNetworkInfo.dhcpGateway();
}

void
Connection::Notify(const hal::NetworkInformation& aNetworkInfo)
{
  ConnectionType previousType = mType;

  UpdateFromNetworkInfo(aNetworkInfo);

  if (previousType == mType) {
    return;
  }

  DispatchTrustedEvent(CHANGE_EVENT_NAME);
}

JSObject*
Connection::WrapObject(JSContext* aCx)
{
  return NetworkInformationBinding::Wrap(aCx, this);
}

} 
} 
} 
