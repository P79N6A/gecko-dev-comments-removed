






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
  aNetworkInfo->bandwidth() = dom::network::kDefaultBandwidth;
  aNetworkInfo->canBeMetered() = dom::network::kDefaultCanBeMetered;
}

} 
} 
