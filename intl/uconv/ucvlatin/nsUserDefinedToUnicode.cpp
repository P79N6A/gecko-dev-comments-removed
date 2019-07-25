




#include "nsUCConstructors.h"
#include "nsUserDefinedToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "userdefined.ut"
};

nsresult
nsUserDefinedToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
