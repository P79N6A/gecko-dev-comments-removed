




#include "nsUCConstructors.h"
#include "nsMacTurkishToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacTurkishToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  static const uint16_t g_MacTurkishMappingTable[] = {
#include "macturki.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACTURKISH, true);
  return CreateOneByteDecoder((uMappingTable*) &g_MacTurkishMappingTable,
                            aOuter, aIID, aResult);
}
