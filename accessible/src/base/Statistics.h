






































#ifndef A11Y_STATISTICS_H_
#define A11Y_STATISTICS_H_

#include "mozilla/Telemetry.h"

namespace mozilla {
namespace a11y {
namespace statistics {

  inline void A11yInitialized()
    { Telemetry::Accumulate(Telemetry::A11Y_INSTANTIATED, true); }

  


  inline void ISimpleDOMUsed()
    { Telemetry::Accumulate(Telemetry::ISIMPLE_DOM_USAGE, 1); }

} 
} 
} 

#endif

