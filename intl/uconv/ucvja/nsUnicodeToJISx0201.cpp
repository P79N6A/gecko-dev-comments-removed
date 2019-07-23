




































#include "nsUnicodeToJISx0201.h"
#include "nsUCVJADll.h"
#include "nsUCConstructors.h"

NS_METHOD
nsUnicodeToJISx0201Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  return CreateTableEncoder(u1ByteCharset,
                            (uMappingTable*) &g_uf0201Mapping, 1,
                            aOuter, aIID, aResult);
}

