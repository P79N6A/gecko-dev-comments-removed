





#ifndef RootCertificateTelemetryUtils_h
#define RootCertificateTelemetryUtils_h

#include "mozilla/Telemetry.h"
#include "certt.h"

namespace mozilla { namespace psm {

void
AccumulateTelemetryForRootCA(mozilla::Telemetry::ID probe, const CERTCertificate* cert);

} 
} 

#endif 
