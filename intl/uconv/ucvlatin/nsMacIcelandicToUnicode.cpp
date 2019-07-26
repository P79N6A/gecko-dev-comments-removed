




#include "nsUCConstructors.h"
#include "nsMacIcelandicToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacIcelandicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "macicela.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACICELANDIC, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
