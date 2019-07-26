





#include "Platform.h"

#include "AccEvent.h"
#include "Compatibility.h"
#include "nsAccessNodeWrap.h"
#include "nsWinUtils.h"

using namespace mozilla;
using namespace mozilla::a11y;

void
a11y::PlatformInit()
{
  Compatibility::Init();

  nsWinUtils::MaybeStartWindowEmulation();
}

void
a11y::PlatformShutdown()
{
  NS_IF_RELEASE(nsAccessNodeWrap::gTextEvent);
  ::DestroyCaret();

  nsWinUtils::ShutdownWindowEmulation();
}

