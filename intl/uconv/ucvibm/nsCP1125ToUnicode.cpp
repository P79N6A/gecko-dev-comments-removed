



#include "nsUCConstructors.h"
#include "nsCP1125ToUnicode.h"







nsresult
nsCP1125ToUnicodeConstructor(nsISupports* aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_utMappingTable[] = {
#include "cp1125.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_utMappingTable,
                              aOuter, aIID, aResult);
}
