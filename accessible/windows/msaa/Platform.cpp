





#include "Platform.h"

#include "AccEvent.h"
#include "Compatibility.h"
#include "HyperTextAccessibleWrap.h"
#include "nsWinUtils.h"
#include "mozilla/a11y/ProxyAccessible.h"
#include "ProxyWrappers.h"

#include "mozilla/ClearOnShutdown.h"

using namespace mozilla;
using namespace mozilla::a11y;

void
a11y::PlatformInit()
{
  Compatibility::Init();

  nsWinUtils::MaybeStartWindowEmulation();
  ClearOnShutdown(&HyperTextAccessibleWrap::sLastTextChangeAcc);
  ClearOnShutdown(&HyperTextAccessibleWrap::sLastTextChangeString);
}

void
a11y::PlatformShutdown()
{
  ::DestroyCaret();

  nsWinUtils::ShutdownWindowEmulation();
}

void
a11y::ProxyCreated(ProxyAccessible* aProxy, uint32_t)
{
  ProxyAccessibleWrap* wrapper = new ProxyAccessibleWrap(aProxy);
  wrapper->AddRef();
  aProxy->SetWrapper(reinterpret_cast<uintptr_t>(wrapper));
}

void
a11y::ProxyDestroyed(ProxyAccessible* aProxy)
{
  ProxyAccessibleWrap* wrapper =
    reinterpret_cast<ProxyAccessibleWrap*>(aProxy->GetWrapper());
  wrapper->Shutdown();
  aProxy->SetWrapper(0);
  wrapper->Release();
}

void
a11y::ProxyEvent(ProxyAccessible*, uint32_t)
{
}
