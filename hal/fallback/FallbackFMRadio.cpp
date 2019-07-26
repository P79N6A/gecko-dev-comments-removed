





#include "Hal.h"

namespace mozilla {
namespace hal_impl {

void
EnableFMRadio(const hal::FMRadioSettings& aInfo)
{}

void
DisableFMRadio()
{}

void
FMRadioSeek(const hal::FMRadioSeekDirection& aDirection)
{}

void
GetFMRadioSettings(hal::FMRadioSettings* aInfo)
{
  aInfo->upperLimit() = 0;
  aInfo->lowerLimit() = 0;
  aInfo->spaceType() = 0;
  aInfo->preEmphasis() = 0;
}

void
SetFMRadioFrequency(const uint32_t frequency)
{}

uint32_t
GetFMRadioFrequency()
{
  return 0;
}

bool
IsFMRadioOn()
{
  return false;
}

uint32_t
GetFMRadioSignalStrength()
{
  return 0;
}

void
CancelFMRadioSeek()
{}

} 
} 
