




#include "nsUCConstructors.h"
#include "nsMacGujaratiToUnicode.h"




nsresult
nsMacGujaratiToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
   static const uint16_t g_utMappingTable[] = {
#include "macgujarati.ut"
   };

   Telemetry::Accumulate(Telemetry::DECODER_INSTANTIATED_MACGUJARATI, true);
   return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                               aOuter, aIID, aResult);
}
