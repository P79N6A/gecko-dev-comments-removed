


















#include "nsUCConstructors.h"
#include "nsCP862ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsCP862ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp862.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_IBM862, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

