


















#include "nsUCConstructors.h"
#include "nsCP855ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsCP855ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp855.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_IBM855, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

