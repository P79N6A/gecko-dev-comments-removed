




#include "nsUCConstructors.h"
#include "nsUnicodeToISO88594.h"




static const uint16_t g_ufMappingTable[] = {
#include "8859-4.uf"
};

nsresult
nsUnicodeToISO88594Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

