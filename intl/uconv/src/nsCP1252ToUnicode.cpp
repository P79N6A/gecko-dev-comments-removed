




#include "nsUCConstructors.h"
#include "nsCP1252ToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "cp1252.ut"
};

nsresult
nsCP1252ToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
