


















#include "nsUCConstructors.h"
#include "nsCP852ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsCP852ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp852.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_IBM852, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

