



#include "mozilla/dom/GamepadMonitoring.h"
#include "mozilla/dom/GamepadFunctions.h"
#include "mozilla/dom/PContentParent.h"

namespace mozilla {
namespace dom {

using namespace GamepadFunctions;

void
MaybeStopGamepadMonitoring()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  nsTArray<ContentParent*> t;
  ContentParent::GetAll(t);
  for(uint32_t i = 0; i < t.Length(); ++i) {
    if (t[i]->HasGamepadListener()) {
      return;
    }
  }
  StopGamepadMonitoring();
  ResetGamepadIndexes();
}

} 
} 
