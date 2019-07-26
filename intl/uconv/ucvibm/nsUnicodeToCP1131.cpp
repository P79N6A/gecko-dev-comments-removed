



#include "nsUCConstructors.h"
#include "nsUnicodeToCP1131.h"




nsresult
nsUnicodeToCP1131Constructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "cp1131.uf"
  };

  return CreateTableEncoder(u1ByteCharset, 
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

