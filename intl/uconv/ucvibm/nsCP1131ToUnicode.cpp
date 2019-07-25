



#include "nsUCConstructors.h"
#include "nsCP1131ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1131.ut"
};




nsresult
nsCP1131ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
