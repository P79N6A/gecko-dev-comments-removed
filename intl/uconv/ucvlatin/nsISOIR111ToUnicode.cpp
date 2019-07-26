




#include "nsUCConstructors.h"
#include "nsISOIR111ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsISOIR111ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "iso-ir-111.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_ISOIR111, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
