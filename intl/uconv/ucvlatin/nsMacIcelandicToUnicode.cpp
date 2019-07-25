




#include "nsUCConstructors.h"
#include "nsMacIcelandicToUnicode.h"




static const uint16_t g_utMappingTable[] = {
#include "macicela.ut"
};

nsresult
nsMacIcelandicToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                   void **aResult) 
{
  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
