




































#include "nsUCConstructors.h"
#include "nsUnicodeToMacTurkish.h"




static const PRUint16 g_MacTurkishMappingTable[] = {
#include "macturki.uf"
};

NS_METHOD
nsUnicodeToMacTurkishConstructor(nsISupports *aOuter, REFNSIID aIID,
                                 void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_MacTurkishMappingTable, 1,
                            aOuter, aIID, aResult);
}

