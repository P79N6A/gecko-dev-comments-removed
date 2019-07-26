




#include "nsUCConstructors.h"
#include "nsUnicodeToCP1255.h"




nsresult
nsUnicodeToCP1255Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp1255.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

