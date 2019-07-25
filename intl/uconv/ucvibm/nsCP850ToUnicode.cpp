


















#include "nsUCConstructors.h"
#include "nsCP850ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp850.ut"
};




nsresult
nsCP850ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
