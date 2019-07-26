




#include "nsUCConstructors.h"
#include "nsMacGurmukhiToUnicode.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;




nsresult
nsMacGurmukhiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macgurmukhi.ut"
   };

   Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACGURMUKHI, true);
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
