




































#include "nsUCConstructors.h"
#include "nsUnicodeToISO885915.h"




static const PRUint16 g_ufMappingTable[] = {
#include "8859-15.uf"
};

NS_METHOD
nsUnicodeToISO885915Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

