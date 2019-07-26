




#include "nsUCConstructors.h"
#include "nsMacCroatianToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacCroatianToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "maccroat.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACCROATIAN, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

