




#include "nsUCConstructors.h"
#include "nsUserDefinedToUnicode.h"




nsresult
nsUserDefinedToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                  void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "userdefined.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
