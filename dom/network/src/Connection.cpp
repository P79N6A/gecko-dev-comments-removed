




#include <limits>
#include "mozilla/Hal.h"
#include "mozilla/dom/network/Connection.h"
#include "mozilla/dom/MozConnectionBinding.h"
#include "nsIDOMClassInfo.h"
#include "mozilla/Preferences.h"
#include "nsDOMEvent.h"
#include "Constants.h"





#define CHANGE_EVENT_NAME NS_LITERAL_STRING("change")

namespace mozilla {
namespace dom {
namespace network {

const char* Connection::sMeteredPrefName     = "dom.network.metered";
const bool  Connection::sMeteredDefaultValue = false;

NS_IMPL_QUERY_INTERFACE_INHERITED1(Connection, nsDOMEventTargetHelper,
                                   nsINetworkProperties)



NS_IMPL_ADDREF_INHERITED(dom::network::Connection, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(dom::network::Connection, nsDOMEventTargetHelper)

Connection::Connection()
  : mCanBeMetered(kDefaultCanBeMetered)
  , mBandwidth(kDefaultBandwidth)
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

double
Connection::Bandwidth() const
{
  if (mBandwidth == kDefaultBandwidth) {
    return std::numeric_limits<double>::infinity();
  }

  return mBandwidth;
}

bool
Connection::Metered() const
{
  if (!mCanBeMetered) {
    return false;
  }

  return Preferences::GetBool(sMeteredPrefName, sMeteredDefaultValue);
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
  mBandwidth = aNetworkInfo.bandwidth();
  mCanBeMetered = aNetworkInfo.canBeMetered();
  mIsWifi = aNetworkInfo.isWifi();
  mDHCPGateway = aNetworkInfo.dhcpGateway();
}

void
Connection::Notify(const hal::NetworkInformation& aNetworkInfo)
{
  double previousBandwidth = mBandwidth;
  bool previousCanBeMetered = mCanBeMetered;

  UpdateFromNetworkInfo(aNetworkInfo);

  if (previousBandwidth == mBandwidth &&
      previousCanBeMetered == mCanBeMetered) {
    return;
  }

  DispatchTrustedEvent(CHANGE_EVENT_NAME);
}

JSObject*
Connection::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MozConnectionBinding::Wrap(aCx, aScope, this);
}

} 
} 
} 
