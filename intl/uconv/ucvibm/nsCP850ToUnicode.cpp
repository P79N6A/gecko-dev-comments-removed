


















#include "nsUCConstructors.h"
#include "nsCP850ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;







nsresult
nsCP850ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp850.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_IBM850, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
