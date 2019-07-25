




#include "nsUCConstructors.h"
#include "nsUnicodeToISO88599.h"




static const uint16_t g_ufMappingTable[] = {
#include "8859-9.uf"
};

nsresult
nsUnicodeToISO88599Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}
