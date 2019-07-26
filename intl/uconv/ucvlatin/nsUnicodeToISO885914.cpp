




#include "nsUCConstructors.h"
#include "nsUnicodeToISO885914.h"




nsresult
nsUnicodeToISO885914Constructor(nsISupports *aOuter, REFNSIID aIID,
                                void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "8859-14.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

