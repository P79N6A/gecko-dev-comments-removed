






#include "Hal.h"
#include "mozilla/dom/network/Constants.h"

namespace mozilla {
namespace hal_impl {

void
EnableNetworkNotifications()
{}

void
DisableNetworkNotifications()
{}

void
GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo)
{
  aNetworkInfo->type() = dom::network::kDefaultType;
  aNetworkInfo->isWifi() = dom::network::kDefaultIsWifi;
  aNetworkInfo->dhcpGateway() = dom::network::kDefaultDHCPGateway;
}

} 
} 
