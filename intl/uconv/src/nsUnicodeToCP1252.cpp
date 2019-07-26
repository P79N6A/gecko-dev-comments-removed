




#include "nsUCConstructors.h"
#include "nsUnicodeToCP1252.h"




nsresult
nsUnicodeToCP1252Constructor(nsISupports *aOuter, REFNSIID aIID,
                             void **aResult)
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp1252.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

