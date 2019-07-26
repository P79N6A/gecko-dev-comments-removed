




#include "nsUCConstructors.h"
#include "nsUnicodeToMacTurkish.h"




nsresult
nsUnicodeToMacTurkishConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  static const uint16_t g_MacTurkishMappingTable[] = {
#include "macturki.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacTurkishMappingTable, 1,
                            aOuter, aIID, aResult);
}

