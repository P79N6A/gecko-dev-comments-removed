




#include "nsUCConstructors.h"
#include "nsUnicodeToCP1258.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp1258.uf"
};

nsresult
nsUnicodeToCP1258Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

