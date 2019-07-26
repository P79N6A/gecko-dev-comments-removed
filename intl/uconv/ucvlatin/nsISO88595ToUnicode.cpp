




#include "nsUCConstructors.h"
#include "nsISO88595ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsISO88595ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "8859-5.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_ISO_8859_5, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}

