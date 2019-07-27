



#include "mozilla/dom/GamepadFunctions.h"
#include "mozilla/dom/GamepadService.h"
#include "nsHashKeys.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/unused.h"

namespace mozilla {
namespace dom {
namespace GamepadFunctions {

namespace {

uint32_t gGamepadIndex = 0;
}

template<class T>
void
NotifyGamepadChange(const T& aInfo)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  GamepadChangeEvent e(aInfo);
  nsTArray<ContentParent*> t;
  ContentParent::GetAll(t);
  for(uint32_t i = 0; i < t.Length(); ++i) {
    unused << t[i]->SendGamepadUpdate(e);
  }
  
  if (GamepadService::IsServiceRunning()) {
    nsRefPtr<GamepadService> svc = GamepadService::GetService();
    svc->Update(e);
  }
}

uint32_t
AddGamepad(const char* aID,
           GamepadMappingType aMapping,
           uint32_t aNumButtons, uint32_t aNumAxes)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  int index = gGamepadIndex;
  gGamepadIndex++;
  GamepadAdded a(NS_ConvertUTF8toUTF16(nsDependentCString(aID)), index,
                 (uint32_t)aMapping, aNumButtons, aNumAxes);
  gGamepadIndex++;
  NotifyGamepadChange<GamepadAdded>(a);
  return index;
}

void
RemoveGamepad(uint32_t aIndex)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  GamepadRemoved a(aIndex);
  NotifyGamepadChange<GamepadRemoved>(a);
}

void
NewButtonEvent(uint32_t aIndex, uint32_t aButton,
               bool aPressed, double aValue)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  GamepadButtonInformation a(aIndex, aButton, aPressed, aValue);
  NotifyGamepadChange<GamepadButtonInformation>(a);
}

void
NewButtonEvent(uint32_t aIndex, uint32_t aButton,
               bool aPressed)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  
  NewButtonEvent(aIndex, aButton, aPressed, aPressed ? 1.0L : 0.0L);
}

void
NewAxisMoveEvent(uint32_t aIndex, uint32_t aAxis,
                 double aValue)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  GamepadAxisInformation a(aIndex, aAxis, aValue);
  NotifyGamepadChange<GamepadAxisInformation>(a);
}

void
ResetGamepadIndexes()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  gGamepadIndex = 0;
}

} 
} 
} 
