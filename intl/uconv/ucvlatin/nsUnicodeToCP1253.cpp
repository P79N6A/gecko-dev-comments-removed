




#include "nsUCConstructors.h"
#include "nsUnicodeToCP1253.h"




nsresult
nsUnicodeToCP1253Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp1253.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

