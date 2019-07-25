



#include "nsUCConstructors.h"
#include "nsUnicodeToCP869.h"




static const PRUint16 g_ufMappingTable[] = {
#include "cp869.uf"
};

nsresult
nsUnicodeToCP869Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

