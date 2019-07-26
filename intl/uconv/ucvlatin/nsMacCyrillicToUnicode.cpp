




#include "nsUCConstructors.h"
#include "nsMacCyrillicToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacCyrillicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "maccyril.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACCYRILLIC, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

