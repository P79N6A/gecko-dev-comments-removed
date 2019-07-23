



















































#include "nsUCConstructors.h"
#include "nsUnicodeToCP864i.h"



static const PRUint16 g_ufMappingTable[] = {
#include "864i.uf"
};

NS_METHOD
nsUnicodeToCP864iConstructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}


