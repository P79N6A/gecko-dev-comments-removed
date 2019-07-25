




































#include "Hal.h"

namespace mozilla {
namespace hal_impl {

void
Vibrate(const nsTArray<uint32>& pattern, const hal::WindowIdentifier &)
{}

void
CancelVibrate(const hal::WindowIdentifier &)
{}

bool
GetScreenEnabled()
{
  return true;
}

void
SetScreenEnabled(bool enabled)
{}

double
GetScreenBrightness()
{
  return 1;
}

void
SetScreenBrightness(double brightness)
{}

} 
} 
