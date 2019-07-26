




#include "nsUCConstructors.h"
#include "nsUnicodeToKOI8U.h"




nsresult
nsUnicodeToKOI8UConstructor(nsISupports *aOuter, REFNSIID aIID,
                            void **aResult) 
{
  static const uint16_t g_ufMappingTable[] = {
#include "koi8u.uf"
  };

  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_ufMappingTable, 1,
                            aOuter, aIID, aResult);
}

