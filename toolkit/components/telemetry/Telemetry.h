





































#ifndef Telemetry_h__
#define Telemetry_h__

namespace mozilla {
namespace Telemetry {

enum ID {
#define HISTOGRAM(name, a, b, c, d, e, f) name,

#include "TelemetryHistograms.h"

#undef HISTOGRAM
HistogramCount
};







void Accumulate(ID id, PRUint32 sample);

} 
} 
#endif 
