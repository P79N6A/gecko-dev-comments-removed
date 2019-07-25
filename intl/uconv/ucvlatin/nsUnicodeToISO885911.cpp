





#include "nsUCConstructors.h"
#include "nsUnicodeToISO885911.h"




static const uint16_t g_ufMappingTable[] = {
#include "8859-11.uf"
};

nsresult
nsUnicodeToISO885911Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

