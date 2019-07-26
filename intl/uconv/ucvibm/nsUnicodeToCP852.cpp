


















#include "nsUCConstructors.h"
#include "nsUnicodeToCP852.h"




nsresult
nsUnicodeToCP852Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp852.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

