


















#include "nsUCConstructors.h"
#include "nsCP857ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsCP857ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp857.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_IBM857, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

