




#include "nsVISCIIToUnicode.h"
#include "nsUCConstructors.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsVISCIIToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "viscii.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_VISCII, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
