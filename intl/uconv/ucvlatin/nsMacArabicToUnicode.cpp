




#include "nsUCConstructors.h"
#include "nsMacArabicToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacArabicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macarabic.ut"
   };

   Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACARABIC, true);
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
