




#include "nsUCConstructors.h"
#include "nsKOI8RToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsKOI8RToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "koi8r.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_KOI8R, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

