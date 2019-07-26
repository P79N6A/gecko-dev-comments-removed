




#include "nsUCConstructors.h"
#include "nsMacCEToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacCEToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_MacCEMappingTable[] = {
#include "macce.ut"
  };

  Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACCE, true);
  return CreateOneByteDecoder((uMappingTable*) &g_MacCEMappingTable,
                            aOuter, aIID, aResult);
}

