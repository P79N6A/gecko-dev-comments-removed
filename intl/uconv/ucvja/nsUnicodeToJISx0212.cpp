




































#include "nsUnicodeToJISx0212.h"
#include "nsUCVJADll.h"
#include "nsUCConstructors.h"




NS_METHOD
nsUnicodeToJISx0212Constructor(nsISupports *aOuter, REFNSIID aIID,
                               void **aResult)
{
  return CreateTableEncoder(u2BytesCharset,
                            (uMappingTable*) g_uf0212Mapping,
                            2 ,
                            aOuter, aIID, aResult);
}

