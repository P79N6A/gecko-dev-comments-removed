




#include "nsUCConstructors.h"
#include "nsMacGreekToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacGreekToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_MacGreekMappingTable[] = {
#include "macgreek.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACGREEK, true);
  return CreateOneByteDecoder((uMappingTable*) &g_MacGreekMappingTable,
                            aOuter, aIID, aResult);
}

