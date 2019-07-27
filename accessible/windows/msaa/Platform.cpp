





#include "Platform.h"

#include "AccEvent.h"
#include "Compatibility.h"
#include "HyperTextAccessibleWrap.h"
#include "nsWinUtils.h"

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

