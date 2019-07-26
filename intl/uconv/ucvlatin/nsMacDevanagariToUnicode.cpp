




#include "nsUCConstructors.h"
#include "nsMacDevanagariToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacDevanagariToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                    void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macdevanaga.ut"
   };

   Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACDEVANAGARI, true);
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
