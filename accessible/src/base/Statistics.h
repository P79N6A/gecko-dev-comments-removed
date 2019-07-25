





#ifndef A11Y_STATISTICS_H_
#define A11Y_STATISTICS_H_

#include "mozilla/Telemetry.h"

namespace mozilla {
namespace a11y {
namespace statistics {

  inline void A11yInitialized()
    { Telemetry::Accumulate(Telemetry::A11Y_INSTANTIATED_FLAG, true); }

  inline void A11yConsumers(PRUint32 aConsumer)
    { Telemetry::Accumulate(Telemetry::A11Y_CONSUMERS, aConsumer); }

  


  inline void ISimpleDOMUsed()
    { Telemetry::Accumulate(Telemetry::A11Y_ISIMPLEDOM_USAGE_FLAG, true); }

  


  inline void IAccessibleTableUsed()
    { Telemetry::Accumulate(Telemetry::A11Y_IATABLE_USAGE_FLAG, true); }

  


  inline void XFormsAccessibleUsed()
    { Telemetry::Accumulate(Telemetry::A11Y_XFORMS_USAGE_FLAG, true); }

} 
} 
} 

#endif

