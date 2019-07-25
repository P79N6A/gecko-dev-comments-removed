






































#include "Hal.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/hal_sandbox/PHalChild.h"
#include "mozilla/hal_sandbox/PHalParent.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::hal;

namespace mozilla {
namespace hal_sandbox {

static PHalChild* sHal;
static PHalChild*
Hal()
{
  if (!sHal) {
    sHal = ContentChild::GetSingleton()->SendPHalConstructor();
  }
  return sHal;
}

void
Vibrate(const nsTArray<uint32>& pattern)
{
  AutoInfallibleTArray<uint32, 8> p(pattern);
  Hal()->SendVibrate(p);
}

class HalParent : public PHalParent {
public:
  NS_OVERRIDE virtual bool
  RecvVibrate(const InfallibleTArray<unsigned int>& pattern) {
    
    
    
    hal::Vibrate(pattern);
    return true;
  }
};

class HalChild : public PHalChild {
public:
};

PHalChild* CreateHalChild() {
  return new HalChild();
}

PHalParent* CreateHalParent() {
  return new HalParent();
}

} 
} 
