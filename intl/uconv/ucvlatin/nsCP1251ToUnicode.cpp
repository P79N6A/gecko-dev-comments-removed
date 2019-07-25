




#include "nsUCConstructors.h"
#include "nsCP1251ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1251.ut"
};

nsresult
nsCP1251ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
