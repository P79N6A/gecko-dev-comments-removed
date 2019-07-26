




#include "nsUCConstructors.h"
#include "nsMacFarsiToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacFarsiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macfarsi.ut"
   };

   Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACFARSI, true);
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
