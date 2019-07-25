




































#include "Utils.h"
#include "mozilla/Preferences.h"

namespace mozilla {
namespace dom {
namespace network {

 bool
IsAPIEnabled()
{
  return Preferences::GetBool("dom.network.enabled", true);
}

} 
} 
} 
