




#include "nsUCConstructors.h"
#include "nsARMSCII8ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsARMSCII8ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "armscii.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_ARMSCII8, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

