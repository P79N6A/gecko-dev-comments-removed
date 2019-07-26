




#include "nsUCConstructors.h"
#include "nsMacHebrewToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacHebrewToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "machebrew.ut"
   };

   Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACHEBREW, true);
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
