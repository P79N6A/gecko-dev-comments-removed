




#include "nsUCConstructors.h"
#include "nsMacRomanianToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacRomanianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "macro.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACROMANIAN, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
