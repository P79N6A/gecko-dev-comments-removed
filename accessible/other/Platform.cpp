





#include "Platform.h"

using namespace mozilla;
using namespace mozilla::a11y;

void
a11y::PlatformInit()
{
}

void
a11y::PlatformShutdown()
{
}

void
a11y::ProxyCreated(ProxyAccessible*, uint32_t)
{
}

void
a11y::ProxyDestroyed(ProxyAccessible*)
{
}

void
a11y::ProxyEvent(ProxyAccessible*, uint32_t)
{
}

void
a11y::ProxyStateChangeEvent(ProxyAccessible*, uint64_t, bool)
{
}

void
a11y::ProxyCaretMoveEvent(ProxyAccessible* aTarget, int32_t aOffset)
{
}

void
a11y::ProxyTextChangeEvent(ProxyAccessible*, const nsString&, int32_t, uint32_t,
                     bool, bool)
{
}
