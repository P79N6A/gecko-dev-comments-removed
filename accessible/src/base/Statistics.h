







































#ifndef A11Y_STATISTICS_H_
#define A11Y_STATISTICS_H_

#include "mozilla/Telemetry.h"

namespace mozilla {
namespace a11y {
namespace statistics {

  inline void A11yInitialized()
    { Telemetry::Accumulate(Telemetry::A11Y_INSTANTIATED, 1); }

  inline void A11yConsumers(PRUint32 aConsumer)
    { Telemetry::Accumulate(Telemetry::A11Y_CONSUMERS, aConsumer); }

  


  inline void ISimpleDOMUsed()
  {
    static bool firstTime = true;
    if (firstTime) {
      Telemetry::Accumulate(Telemetry::ISIMPLE_DOM_USAGE, 1);
      firstTime = false;
    }
  }

  


  inline void IAccessibleTableUsed()
    { Telemetry::Accumulate(Telemetry::IACCESSIBLE_TABLE_USAGE, 1); }

  


  inline void XFormsAccessibleUsed()
    { Telemetry::Accumulate(Telemetry::XFORMS_ACCESSIBLE_USED, 1); }

} 
} 
} 

#endif

