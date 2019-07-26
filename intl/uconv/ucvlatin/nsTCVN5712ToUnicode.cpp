




#include "nsUCConstructors.h"
#include "nsTCVN5712ToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsTCVN5712ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "tcvn5712.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_VIETTCVN5712, true);
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
