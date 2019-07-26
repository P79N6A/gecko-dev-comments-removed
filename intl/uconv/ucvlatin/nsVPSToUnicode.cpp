




#include "nsVPSToUnicode.h"
#include "nsUCConstructors.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsVPSToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                          void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "vps.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_VIETVPS, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

