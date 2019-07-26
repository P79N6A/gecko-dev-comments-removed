




#include "nsUCConstructors.h"
#include "nsUnicodeToISO88595.h"




nsresult
nsUnicodeToISO88595Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "8859-5.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

