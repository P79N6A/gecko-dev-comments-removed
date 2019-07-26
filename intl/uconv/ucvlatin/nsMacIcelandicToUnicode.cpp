




#include "nsUCConstructors.h"
#include "nsMacIcelandicToUnicode.h"




nsresult
nsMacIcelandicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "macicela.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
