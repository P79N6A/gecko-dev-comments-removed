




#include "nsUCConstructors.h"
#include "nsMacTurkishToUnicode.h"




nsresult
nsMacTurkishToUnicodeConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  static const uint16_t g_MacTurkishMappingTable[] = {
#include "macturki.ut"
  };

  return CreateOneByteDecoder((uMappingTable*) &g_MacTurkishMappingTable,
                            aOuter, aIID, aResult);
}
