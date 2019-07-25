




#include "nsUCConstructors.h"
#include "nsUnicodeToMacTurkish.h"




static const uint16_t g_MacTurkishMappingTable[] = {
#include "macturki.uf"
};

nsresult
nsUnicodeToMacTurkishConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacTurkishMappingTable, 1,
                            aOuter, aIID, aResult);
}

