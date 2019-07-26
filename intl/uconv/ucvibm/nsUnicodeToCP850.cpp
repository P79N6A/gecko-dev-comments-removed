


















#include "nsUCConstructors.h"
#include "nsUnicodeToCP850.h"




nsresult
nsUnicodeToCP850Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp850.uf"
  };

  return CreateTableEncoder(u1ByteCharset, 
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

