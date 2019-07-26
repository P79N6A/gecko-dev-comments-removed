




#include "nsUCConstructors.h"
#include "nsKOI8UToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsKOI8UToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "koi8u.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_KOI8U, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
